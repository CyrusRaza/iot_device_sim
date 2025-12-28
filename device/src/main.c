#ifndef MQTT_H
#define MQTT_H


#include "some_header.h"
#include "transport.h"
#include "transport_select.h"


// header contents

#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <time.h>


static void sleep_ms(long ms){
    struct timespec ts = { .tv_sec = ms/1000, .tv_nsec = (ms%1000)*1000000L };
    nanosleep(&ts, NULL);
}


config_pr* argument_parse(int argc, char* argv[])
{
    
    for(int i=1; i < argc;i++)
    {

        if(argv[i] && (strcmp(argv[i], "-h") ==0))
        {
            printf("the code expects 3 command line arguments in order: device_id(int), transport_type(mqtt, http or webs) and port(4 digit int)\n");
            return NULL;
        }
    }

    if(argc<4)
    {
        printf("we need more command line arguments\n");
        return NULL;
    }
    if(strlen(argv[3]) !=4 || strlen(argv[2]) !=4)
    {
        printf("transport_types or port needs to be 4 characters long. Use -h to check help\n");
        return NULL;
    }
    for(int i=0;i< strlen(argv[1]);i++)
    {
        if(!isdigit(argv[1][i]))
        {
            printf("device id needs to be all digits\n");
            return NULL;
        }
    }
    
    for(int i=0;i < strlen(argv[3]);i++)
    {
        if(!isdigit(argv[3][i]))
        {
            printf("port needs to be all digits\n");
            return NULL;
        }

        
    }
    if(!((strcmp(argv[2], transport_t[0]) == 0) || (strcmp(argv[2], transport_t[1])==0) || (strcmp(argv[2], transport_t[2]) == 0 )))
        {
            printf("transport types in wrong format, check help to find usage\n");
            return NULL;
        }
    struct config_pr* con = malloc(sizeof(*con));
    
    memset(con->device_id,0,sizeof(con->device_id)+1);
    strncpy(con->device_id, argv[1], sizeof(argv[1]));
    
    memset(con->transport_typ,0,sizeof(con->transport_typ)+1);
    strncpy(con->transport_typ, argv[2], 4);
    
    if(strcmp(con->transport_typ, "mqtt")==0)
    {
        con->transport=0;
    }
    else if (strcmp(con->transport_typ, "http")==0)
    {
        con->transport=1;
    }
    else if (strcmp(con->transport_typ, "webs")==0)
    {
        con->transport=2;
    }   

    con->port = atoi(argv[3]);
    
    printf("config: \tdevice id = %s \ttransport type = %s \tport= %i\n",con->device_id, con->transport_typ, con->port);
    
    return con;
}

int main(int argc, char* argv[])
{
    config_pr* conf = argument_parse(argc, argv);
    if (conf == NULL)
    {
        printf("what\n");
        return 1;
    }
    transport_type* tra = transport_sel(*conf, "127.0.0.1", conf->port, "hello");
    if (tra== NULL)
    {
        return 1;
    }
    
    int rc =0;
    rc = tra->vt->init(tra);
    if(rc !=0)
    {
        return 1;
    }
    rc = tra->vt->connect(tra);
    if(rc !=0)
    {
        tra->vt->destroy(tra);
        return 1;
    }

    for(int i=0; i<20; i++)
    {

        tra->vt->publish(tra);
        sleep_ms(5000);
    }
    tra->vt->destroy(tra);
    free(conf);
    conf = NULL;
    return 0;
}
