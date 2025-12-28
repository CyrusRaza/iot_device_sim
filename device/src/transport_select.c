#ifndef MQTT_H
#define MQTT_H

#include "transport_select.h"
#include "some_header.h"


#endif
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


transport_type* transport_sel(config_pr conf, char* host, int port, char* msg)
{
    transport_type* temp = NULL;
    switch (conf.transport){
        case 0:
        temp = mqtt_constructor(host, port, msg, conf.device_id);
        printf("mqqt selected\n");
        break;
        case 1:
        //temp = http_constructor(host,port);
        break;
        case 2:
        //temp = webs_constructor(host, port);
        break;
        default:
        printf("error with enum transport type.\n");
        temp = NULL;
        break;
    }
    return temp;
}
