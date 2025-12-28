#ifndef MQTT_H
#define MQTT_H

#include "transport.h"
#include "transport_select.h"

#endif


#ifndef CHARARRAY
#define CHARARRAY
static const char* transport_t[] = {"mqtt", "http", "webs"};
#endif

enum types_transport {
    MQTT,
    HTTP,
    WEBS,
};

typedef struct config_pr{
    char device_id[10];
    char transport_typ[5];
    enum types_transport transport;
    int port;
}config_pr;