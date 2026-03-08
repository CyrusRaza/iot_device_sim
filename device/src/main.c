#ifndef MQTT_H
#define MQTT_H


#include "some_header.h"
#include "transport.h"
#include "transport_select.h"
#include "shared_mem.h"
#include "string_parser.h"


// header contents

#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <threads.h>





config_pr* argument_parse(int argc, char* argv[])
{
    
    for(int i=1; i < argc;i++)
    {

        if(argv[i] && (strcmp(argv[i], "-h") ==0))
        {
            printf("fn argument_parser:the code expects 3 command line arguments in order: device_id(int), transport_type(mqtt, http or webs) and port(4 digit int)\n");
            return NULL;
        }
    }

    if(argc<4)
    {
        printf("fn argument_parser: we need more command line arguments\n");
        return NULL;
    }
    if(strlen(argv[3]) !=4 || strlen(argv[2]) !=4)
    {
        printf("fn argument_parser: transport_types or port needs to be 4 characters long. Use -h to check help\n");
        return NULL;
    }
    for(int i=0;i< strlen(argv[1]);i++)
    {
        if(!isdigit(argv[1][i]))
        {
            printf("fn argument_parser: device id needs to be all digits\n");
            return NULL;
        }
    }
    
    for(int i=0;i < strlen(argv[3]);i++)
    {
        if(!isdigit(argv[3][i]))
        {
            printf("fn argument_parser: port needs to be all digits\n");
            return NULL;
        }

        
    }
    if(!((strcmp(argv[2], transport_t[0]) == 0) || (strcmp(argv[2], transport_t[1])==0) || (strcmp(argv[2], transport_t[2]) == 0 )))
        {
            printf("fn argument_parser: transport types in wrong format, check help to find usage\n");
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
    
    printf("fn argument_parser: config: \tdevice id = %s \ttransport type = %s \tport= %i\n",con->device_id, con->transport_typ, con->port);
    
    return con;
}

int main(int argc, char* argv[])
{

    sem_unlink(SEM_OVERWRITE);
    sem_unlink(SEM_UNDERWRITE);


    sem_t* sem_over = sem_open(SEM_OVERWRITE, O_CREAT, 0666, BUFFER_SIZE);
    if (sem_over == SEM_FAILED)
    {
        printf("fn main: Semephore open failed from main\n");
        exit;
    }

    sem_t* sem_under = sem_open(SEM_UNDERWRITE, O_CREAT, 0666, 0);
    if (sem_under == SEM_FAILED)
    {
        printf("fn main: Semephore under failed from main\n");
        exit;
    }
    init_shared_lock();
    config_pr* conf = argument_parse(argc, argv);
    if (conf == NULL)
    {
        printf("fn main: faulty arguments provided. \n");
        return 1;
    }
    
    thrd_t other_thread;
    shared_mem* queue = queue_create();

    printf("fn main: write_ind %i, read_ind %i\n", queue->write_ind, queue->read_ind);
    if ((thrd_create(&other_thread, string_parser, (void *)&queue)) != thrd_success)
    {
        
        free(conf);
        conf = NULL;
        queue_destroy(queue);
        return 465;
    }
    
    transport_type* tra = transport_sel(*conf, "127.0.0.1", conf->port, "hello", (void *)&queue);
    if (tra== NULL)
    {
        return 1;
    }

    
    int rc = 0;
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
        print_commands();
        
        tra->vt->publish(tra);
        sleep_ms(1000);
        
    }
    
    thrd_join(other_thread, &rc);
    mtx_destroy(&command_vals_mutex);
    tra->vt->destroy(tra);
    queue_destroy(queue);
    free(conf);
    conf = NULL;
    return 0;
}
