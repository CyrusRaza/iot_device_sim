#ifndef MQTT_H
#define MQTT_H


#include "some_header.h"
#include "transport.h"
#include "transport_select.h"
#include "shared_mem.h"

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

static mtx_t mutex;

const char* command_list[NUM_COMMANDS] = {"temp_set", "pub_rate", "some1", "some2", "some3"};
static int command_vals[NUM_COMMANDS] = {0, 0, 0, 0, 0};
static void sleep_ms(long ms){
    struct timespec ts = { .tv_sec = ms/1000, .tv_nsec = (ms%1000)*1000000L };
    nanosleep(&ts, NULL);
}


int check_key(char* key)
{
    for (int i=0; i < NUM_COMMANDS; i++)
    {
        if (strcmp(key, command_list[i])==0)
        {
            printf("fn check_key: match found\n");
            return i;
        }
    }
    return -1;
    
}

int change_val(char* key, char* val, int index)
{
    mtx_lock(&mutex);
    command_vals[index] = val;
    mtx_unlock(&mutex);
}


void remove_spaces (char* restrict str_trimmed, const char* restrict str_untrimmed)
{
  while (*str_untrimmed != '\0')
  {
    if(!isspace(*str_untrimmed))
    {
      *str_trimmed = *str_untrimmed;
      str_trimmed++;
    }
    str_untrimmed++;
  }
  *str_trimmed = '\0';
}



int string_parser(void* obj)
{
    shared_mem* queue = *(shared_mem**) obj;
    printf("fn string_parser: write_ind %i, read_ind %i\n", queue->write_ind, queue->read_ind);
    bool queue_empty = false;
    for(int i = 0; i<25; i++)
    {
        
        //printf("is this working? %i \n", (int)queue_empty);
        queue_empty = queue_is_empty(queue);
        if (!queue_empty)
        {
           // printf("queue not empty biatch\n");
            char msg[BUFFER_SIZE];
            queue_pop(queue, msg);
            //strcpy(msg, (char*) obj);
            char* msg_spc_rm = malloc(strlen(msg));
            remove_spaces(msg_spc_rm, msg);
            
            
            char* key_val = strtok(msg_spc_rm, ",{}");
            char key[20];
            char* val;
            //printf("check1\n");
            while(key_val != NULL)
            {
                val = strchr(key_val, ':');
                //printf("check2 %s\n", key_val);
            
                if (val != NULL)
                {
                    if (val-key_val < 20)
                    {
                        strncpy(key, key_val, val-key_val+1);
                        *(key + (val-key_val)) = '\0';
                        int ret_val = check_key(key);
                        //printf("check3 %s %s \n", key, val);
            
                        if (ret_val!=-1)
                        {
                            printf("fn string_parser: changing %s to %i\n", key, atoi(val+1));
                            change_val(key, atoi(val+1), ret_val);
                        }
                    }
                    else
                    {
                        printf("fn string_parser:key too big\n");
                    }
                }   
                key_val = strtok(NULL, ",{}");
            }
            free(msg_spc_rm);
            msg_spc_rm=NULL;
            
        }
        sleep_ms(1000);
    }
    //printf("Hello from the other side.\n");
    return 123;
}


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
    mtx_init(&mutex, mtx_plain);
    config_pr* conf = argument_parse(argc, argv);
    if (conf == NULL)
    {
        printf("fn main: faulty arguments provided. \n");
        return 1;
    }
    //printf("here for you\n");
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
    //printf("after thread\n");
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
        for(int f = 0; f < NUM_COMMANDS;f++)
        {
            mtx_lock(&mutex);
            printf("fn main: %s: %i\t",command_list[f], command_vals[f]);
            mtx_unlock(&mutex);
        }
        printf("\n");

        tra->vt->publish(tra);
        sleep_ms(1000);
        
    }
    
    thrd_join(other_thread, &rc);
    mtx_destroy(&mutex);
    tra->vt->destroy(tra);
    queue_destroy(queue);
    free(conf);
    conf = NULL;
    return 0;
}
