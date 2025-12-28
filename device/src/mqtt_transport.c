#ifndef MQTT_H
#define MQTT_H

#include "transport.h"
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


typedef struct mqtt_transport_type{
    transport_type base;
    int port;
    char* topic;
    char* message;
    char* host;
    char* device_id;
    struct mosquitto* mosq;
    
}mqtt_transport_type;

int mqtt_init(transport_type* self)
{
    mosquitto_lib_init();
    mqtt_transport_type *m = (mqtt_transport_type *)self;
    
    m->mosq = mosquitto_new("wahts", true, NULL);
    int temp = 0;
    temp = mosquitto_loop_start(m->mosq);
    if (m->mosq == NULL || temp !=0)
    {
        printf("init failed.\n");
        return 1;
    }
    printf("mqtt transport is initialized. \tmqqt.port: %i,\tmqqt.host: %s \tmqqt.message: %s\tdevice id: %s\n", m->port, m->host, m->message, m->device_id);
    return 0;
}

int mqtt_connect(transport_type* self)
{
    int rc = 0;
    mqtt_transport_type *m = (mqtt_transport_type *)self;
    rc = mosquitto_connect(m->mosq, "localhost", m->port, 60);
    if (rc != 0)
    {
        printf("mqtt connection failed\n");
        mosquitto_destroy(m->mosq);
        return 1;
    }
    printf("mqtt: beep boop, connection stablished.\n");
    return 0;
}
    
int mqtt_poll(transport_type* self)
{
    return 0;
}


int mqtt_publish(transport_type* self)
{
    mqtt_transport_type *m = (mqtt_transport_type *)self;
    mosquitto_publish(m->mosq, NULL, "test/topic", 6, m->message, 0, false);
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

transport_type* mqtt_constructor(char* host, int port, char* msg, char* id)
{
    
    mqtt_transport_type *m= calloc(1, sizeof(*m));
    if (!m) {return NULL;}

    m->base.vt = &mqtt_table;
    m->host = host; 
    m->port = port;
    m->message = msg;
    m->device_id = id;
    return &(m->base);
}