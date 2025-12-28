


//#include "some_header.h"

// #ifndef TYPEDEF
// #define TYPEDEF

typedef struct trans transport_type;
typedef struct config_pr config_pr;

//#endif

transport_type* mqtt_constructor(char* host, int port, char* msg, char* id);
//struct transport_type* http_constructor(char* host, int port, char* msg);
//struct transport_type* webs_constructor(char* host, int port, char* msg);

transport_type* transport_sel(struct config_pr conf, char* host, int port, char* msg);
