#ifndef MQTT_H
#define MQTT_H

#include "transport.h"
#include "shared_mem.h"
//#include "transport_select.h"
//#include "some_header.h"


// header contents

#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <mosquitto.h>
#include <semaphore.h>
#include <errno.h>


sem_t* sem_over_mqtt;
sem_t* sem_under_mqtt;


typedef struct mqtt_transport_type{
    transport_type base;
    int port;
    char* topic;
    char* message;
    char* host;
    char* device_id;
    struct mosquitto* mosq;
    void* userdata;
    
}mqtt_transport_type;

void on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg);


static void on_log(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    (void)mosq; (void)userdata;
    fprintf(stderr, "[mosq %d] %s\n", level, str);
}

int mqtt_init(transport_type* self)
{

    mosquitto_lib_init();
    mqtt_transport_type *m = (mqtt_transport_type *)self;
    
    m->mosq = mosquitto_new("wahts", true, m->userdata);
    if (!m->mosq)
    {
        printf("initialization failed\n");
    }
    int temp = 0;

    //int check = mosquitto_threaded_set(m->mosq, true);
    
    sem_over_mqtt = sem_open(SEM_OVERWRITE, 0);
    if (sem_over_mqtt == SEM_FAILED)
    {
        perror("sem_open SEM_OVERWRITE");
        printf("name='%s' errno=%d\n", SEM_OVERWRITE, errno);
        printf("Semephore open failed from mqtt\n");
        exit;
    }

    sem_under_mqtt = sem_open(SEM_UNDERWRITE, 0);
    if (sem_under_mqtt == SEM_FAILED)
    {
        perror("sem_open SEM_OVERWRITE");
        printf("name='%s' errno=%d\n", SEM_UNDERWRITE, errno);
        printf("Semephore under failed from mqtt\n");
        exit;
    }


    
    //mosquitto_log_callback_set(m->mosq, on_log);

    /* Turn on everything (best for debugging) */
    //mosquitto_log_set(mosq, MOSQ_LOG_ALL);
    temp = mosquitto_loop_start(m->mosq);
    if (m->mosq == NULL || temp !=0)
    {
        printf("mqtt_init: init failed. %i\n", temp);
        return 1;
    }
   
    
    printf("mqtt transport is initialized. \tmqqt.port: %i,\tmqqt.host: %s \tmqqt.message: %s\tdevice id: %s\n", m->port, m->host, m->message, m->device_id);
    return 0;
}

int mqtt_connect(transport_type* self)
{
    int rc = 0;
    mqtt_transport_type *m = (mqtt_transport_type *)self;
    rc = mosquitto_connect_async(m->mosq, "127.0.0.1", m->port, 60);
    int temp = 0;
    
    if (rc != 0)
    {
        printf("mqtt connection failed\n");
        mosquitto_destroy(m->mosq);
        return 1;
    }
    printf("mqtt: beep boop, connection stablished.\n");
    
    rc = mosquitto_subscribe(m->mosq, NULL, "test/sub", 0);
    mosquitto_message_callback_set(m->mosq, on_message);
    return 0;


}
    
int mqtt_poll(transport_type* self)
{
    return 0;
}


void on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg)
{
    
    printf("message received on topic %s: %s\n", msg->topic, (char *)msg->payload);
    fflush(stdout);

    sem_wait(sem_over_mqtt);
    queue_push(*(shared_mem**)obj, (char*)msg->payload, (int)strlen(msg->payload)) ;
    sem_post(sem_under_mqtt);
    printf("heassaf\n");
}




int mqtt_publish(transport_type* self)
{
    mqtt_transport_type *m = (mqtt_transport_type *)self;
    mosquitto_publish(m->mosq, NULL, "test/pub", 6, m->message, 0, false);
    printf("mqtt: message sent, koi receive krlo plis.\n");
    return 0;
}

int mqtt_destroy(transport_type* self)
{
    mqtt_transport_type *m = (mqtt_transport_type *)self;
    //free(m->base.vt);
    int temp = 0;
    
    mosquitto_disconnect(m->mosq);
    
    temp = mosquitto_loop_stop(m->mosq, false);
    mosquitto_destroy(m->mosq);
    mosquitto_lib_cleanup();
    free(m);
    printf("mqtt: free free, all mem is free.\n");
    return 0;
}

static const transport_vtable mqtt_table = {
    .init = mqtt_init,
    .connect = mqtt_connect,
    .poll = mqtt_poll,
    .publish = mqtt_publish,
    .destroy = mqtt_destroy,
};

transport_type* mqtt_constructor(char* host, int port, char* msg, char* id, void* userdata)
{
    
    mqtt_transport_type *m= calloc(1, sizeof(*m));
    if (!m) {return NULL;}

    m->base.vt = &mqtt_table;
    m->host = host; 
    m->port = port;
    m->message = msg;
    m->device_id = id;
    m->userdata = userdata;
    return &(m->base);
}