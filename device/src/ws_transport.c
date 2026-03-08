#ifndef WEBS
#define WEBS

#include "transport.h"
#include "shared_mem.h"

//#include "transport_select.h"
//#include "some_header.h"
#endif

//#include "ws_server.h"

#include <libwebsockets.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <threads.h>
#include <semaphore.h>
#include <errno.h>


sem_t* sem_over_ws;
sem_t* sem_under_ws;

void ws_on_message(void *user_ctx, const void *data, size_t len);
void ws_on_connect(void *user_ctx);
void ws_on_disconnect(void *user_ctx);



typedef struct {
    int port;                       // e.g. 9000
              

    
    void *user_ctx;                 // passed back into callbacks
} ws_server_config_t;

int ws_server_thread(void* obj);



struct ws_server_t {
    struct lws_context *ctx;
    struct lws_protocols *protocols;   // 1 + terminator
    ws_server_config_t cfg;

    // A very simple “single pending broadcast” buffer.
    // Good for learning; upgrade to a queue later.
    unsigned char *pending;            // allocated as LWS_PRE + payload
    size_t pending_len;
    int has_pending;

    void* userdata;
    int fatal_error;
}ws_server_t;

typedef struct webs_transport_type{
    transport_type base;
    int port;
    const char *protocol;           
    size_t max_payload;
    
    char* message;
    char* host;
    char* device_id;
    struct ws_server_t* serv;
    void* userdata;
    
}webs_transport_type;

// Per-connection storage (kept minimal here)
struct per_session_data {
    int dummy;
};

static struct ws_server_t *server_from_wsi(struct lws *wsi) {
    struct lws_context *ctx = lws_get_context(wsi);
    return (struct ws_server_t *) lws_context_user(ctx);
}

static int callback_ws(struct lws *wsi,
                       enum lws_callback_reasons reason,
                       void *user,
                       void *in, size_t len)
{
    (void)user;
    struct ws_server_t *s = server_from_wsi(wsi);
    if (!s) return 0;

    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        lwsl_user("WS client connected\n");
        //if (s->cfg.on_connect) s->cfg.on_connect(s->cfg.user_ctx);
        ws_on_connect(in);
        break;

    case LWS_CALLBACK_RECEIVE:
        // App callback gets the received message
        //if (s->cfg.on_message) {
            // Note: LWS gives you message bytes in `in` with length `len`
            // In real apps, validate size/content.
            ws_on_message(s->userdata, in, len);
        //}
        break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
        // When LWS says this connection can write, we send pending broadcast

        if (s->has_pending && s->pending && s->pending_len > 0) {
            int n = lws_write(wsi,
                              s->pending + LWS_PRE,
                              (unsigned int)s->pending_len,
                              LWS_WRITE_TEXT);
            if (n < 0) {
                lwsl_err("lws_write failed\n");
                s->fatal_error = 1;
                return -1; // close this connection
            }
        }
        break;

    case LWS_CALLBACK_CLOSED:
        lwsl_user("WS client disconnected\n");
        //if (s->cfg.on_disconnect) s->cfg.on_disconnect(s->cfg.user_ctx);
        ws_on_disconnect(in);
        break;

    default:
        break;
    }

    return 0;
}


int *webs_init(transport_type* self)
{
    webs_transport_type *ws = (webs_transport_type *) self;

    printf("fn webs_init: whatup\n");
    if (!ws || ws->port <= 0 || !ws->protocol || ws->max_payload == 0) {
        printf("fn webs_init: port %i, protocol %s, max_payload %i, or ws faulty\n", ws->port, ws->protocol, ws->max_payload);
        errno = EINVAL;
        return -1;
    }

    ws->serv = (struct ws_server_t *)calloc(1, sizeof(struct ws_server_t));
    if (!ws->serv)
    {
        printf("fn webs_init: ws_server_t calloc failed\n");
        return NULL;
    }
    

    sem_over_ws = sem_open(SEM_OVERWRITE, 0);
    if (sem_over_ws == SEM_FAILED)
    {
        perror("sem_open SEM_OVERWRITE");
        printf("name='%s' errno=%d\n", SEM_OVERWRITE, errno);
        printf("Semephore open failed from mqtt\n");
        exit;
    }

    sem_under_ws = sem_open(SEM_UNDERWRITE, 0);
    if (sem_under_ws == SEM_FAILED)
    {
        perror("sem_open SEM_OVERWRITE");
        printf("name='%s' errno=%d\n", SEM_UNDERWRITE, errno);
        printf("Semephore under failed from mqtt\n");
        exit;
    }


    ws->serv->protocols = (struct lws_protocols *)calloc(2, sizeof(struct lws_protocols));
    if (!ws->serv->protocols) {
        printf("fn webs_init: lws_protocols calloc failed\n");
        free(ws->serv);
        return -1;
    }

    ws->serv->protocols[0].name = ws->protocol;
    ws->serv->protocols[0].callback = callback_ws;
    ws->serv->protocols[0].per_session_data_size = sizeof(struct per_session_data);
    ws->serv->protocols[0].rx_buffer_size = 0;

    ws->serv->protocols[1].name = NULL;
    ws->serv->protocols[1].callback = NULL;

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    ws->serv->userdata = ws->userdata;

    info.port = ws->port;
    info.protocols = ws->serv->protocols;

    // Store ws_server_t* on the context so callbacks can retrieve it
    info.user = ws->serv;

    ws->serv->ctx = lws_create_context(&info);
    if (!ws->serv->ctx) {
        printf("fn webs_init: lws_create_context failed\n");
        free(ws->serv->protocols);
        free(ws->serv);
        return -1;
    }
    
    thrd_t ws_thread;
    

    if ((thrd_create(&ws_thread, ws_server_thread, (void *)ws)) != thrd_success)
    {
        printf("fn webs_init: thread creation failed\n");
        if (!ws->serv) return 465;
        if (ws->serv->ctx) lws_context_destroy(ws->serv->ctx);
        free(ws->serv->pending);
        free(ws->serv->protocols);
        free(ws->serv);
        
        return -3;
    }

    return 0;
}

int webs_connect(transport_type* self)
{
    webs_transport_type *ws = (webs_transport_type *) self;

    if (!ws->serv || !ws->serv->ctx) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int webs_poll(transport_type* self)
{
    int timeout_ms = 50;
    webs_transport_type *ws = (webs_transport_type *) self;

    if (!ws->serv || !ws->serv->ctx) {
        errno = EINVAL;
        return -1;
    }

    lws_service(ws->serv->ctx, timeout_ms);
//TO CHANGE
    // Tutorial simplification:
    // We free the pending message after one service cycle.
    // Upgrade later: use a queue and track per-client delivery.
    if (ws->serv->has_pending) {
        ws->serv->has_pending = 0;
        free(ws->serv->pending);
        ws->serv->pending = NULL;
        ws->serv->pending_len = 0;
    }
//TO CHANGE
    return ws->serv->fatal_error ? -1 : 0;
}

int webs_send(transport_type* self)
{
    webs_transport_type *ws = (webs_transport_type *) self;
    
    int len = strlen(ws->message);

    if (!ws->serv || !ws->serv->ctx || (!ws->message && len != 0)) {
        errno = EINVAL;
        return -1;
    }

    if (len > ws->max_payload) {
        errno = EMSGSIZE;
        return -1;
    }

    free(ws->serv->pending);
    ws->serv->pending = NULL;
    ws->serv->pending_len = 0;
    ws->serv->has_pending = 0;

    
    ws->serv->pending = (unsigned char *)malloc(LWS_PRE + len);
    if (!ws->serv->pending) return -1;

    if (len > 0) memcpy(ws->serv->pending + LWS_PRE, ws->message, len);

    ws->serv->pending_len = len;
    ws->serv->has_pending = 1;

    
    lws_callback_on_writable_all_protocol(ws->serv->ctx, &ws->serv->protocols[0]);
    return 0;
}

int webs_destroy(transport_type* self)
{
    webs_transport_type *ws = (webs_transport_type *) self;

    if (!ws->serv) return;
    if (ws->serv->ctx) lws_context_destroy(ws->serv->ctx);
    free(ws->serv->pending);
    free(ws->serv->protocols);
    free(ws->serv);
}




static const transport_vtable webs_table = {
    .init = webs_init,
    .connect = webs_connect,
    .poll = webs_poll,
    .publish = webs_send,
    .destroy = webs_destroy,
};

transport_type* webs_constructor(char* host, int port, char* msg, char* id, void* userdata)
{
    
    webs_transport_type *ws = calloc(1, sizeof(*ws));
    if (!ws) {return NULL;}

    ws->base.vt = &webs_table;
    ws->host = host; 
    ws->port = port;
    ws->message = msg;
    ws->device_id = id;
    ws->userdata = userdata;
    ws->max_payload = 4096;
    ws->protocol = "my-proto";
    return &(ws->base);
}

static volatile int g_stop = 0;


static void on_sigint(int sig) {
    (void)sig;
    g_stop = 1;
}


int ws_server_thread(void* obj)
{
    signal(SIGINT, on_sigint);
    
    webs_transport_type *ws = (webs_transport_type *) obj;

    puts("Server listening on ws://127.0.0.1:9000  (protocol: my-proto)");
    puts("Ctrl+C to exit.");


    while (g_stop != 1) {
        if ((ws->base.vt->poll(&(ws->base)) < 0)) {
            puts("[server] fatal error");
            break;
        }
    }

}



void ws_on_message(void* user_ctx, const void *data, size_t len)
{
    printf("[server] received: %.*s\n", (int)len, (const char *)data);

    shared_mem* queue = *(shared_mem**) user_ctx;
    
    sem_wait(sem_over_ws);
    queue_push(queue, (char*)data, len);
    sem_post(sem_under_ws);

}

void ws_on_connect(void *user_ctx) {
    
    puts("[server] client connected");
}

void ws_on_disconnect(void *user_ctx) {
    //(void)user_ctx;
    puts("[server] client disconnected");
}