#include "shared_mem.h"
#include "string_parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <threads.h>
#include <limits.h>
#include <errno.h>

int string_to_int(const char *s, int *out)
{
    char *end;
    long val;

    if (s == NULL || out == NULL) {
        return 1;
    }

    errno = 0;
    val = strtol(s, &end, 10);

    // No digits were found
    if (end == s) {
        return 1;
    }

    // Extra characters after the number
    if (*end != '\0') {
        return 1;
    }

    // Overflow/underflow for long
    if (errno == ERANGE) {
        return 1;
    }

    // Doesn't fit into int
    if (val < INT_MIN || val > INT_MAX) {
        return 1;
    }

    *out = (int)val;
    return 0;
}


const char* command_list[NUM_COMMANDS] = {"temp_set", "pub_rate", "some1", "some2", "some3"};
int command_vals[NUM_COMMANDS] = {0, 0, 0, 0, 0};

mtx_t command_vals_mutex;

void init_shared_lock(void) {
    if (mtx_init(&command_vals_mutex, mtx_plain) != thrd_success) {
        fprintf(stderr, "Failed to initialize mutex\n");
        exit(1);
    }
}


void print_commands()
{
    for(int f = 0; f < NUM_COMMANDS;f++)
    {
        mtx_lock(&command_vals_mutex);
        printf("fn print_commands: %s: %i\t",command_list[f], command_vals[f]);
        mtx_unlock(&command_vals_mutex);
    }
    printf("\n");
        
    
}

void sleep_ms(long ms)
{
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

void change_val(int val, int index)
{
    
    mtx_lock(&command_vals_mutex);
    command_vals[index] = val;
    mtx_unlock(&command_vals_mutex);
    
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

void parser(char* msg)
{
    char* msg_spc_rm = malloc(strlen(msg)+1);
    remove_spaces(msg_spc_rm, msg);
    
    int com_val=0;
    int ret_val=0;
    int ind_val = 0;
    char* key_val = strtok(msg_spc_rm, ",{}");
    char key[20];
    char* val;
    
    while(key_val != NULL)
    {
        val = strchr(key_val, ':');
        
        if (val != NULL)
        {
            if (val-key_val < 20)
            {
                strncpy(key, key_val, val-key_val+1);
                *(key + (val-key_val)) = '\0';
                ind_val = check_key(key);
                ret_val = string_to_int(val+1, &com_val);

                if (ind_val!=-1 && ret_val == 0)
                {   
                    printf("fn string_parser: changing %s to %i\n", key, com_val);
                    change_val(com_val, ind_val);
                }
            }
            else
            {
                printf("fn parser:key too big\n");
            }
        }   
        key_val = strtok(NULL, ",{}");
    }
    free(msg_spc_rm);
    msg_spc_rm=NULL;
        
}

int string_parser(void* obj)
{
    shared_mem* queue = *(shared_mem**) obj;
    printf("fn string_parser: write_ind %i, read_ind %i\n", queue->write_ind, queue->read_ind);
    bool queue_empty = false;
    
    for(int i = 0; i<25; i++)
    {        
        queue_empty = queue_is_empty(queue);
        if (!queue_empty)
        {
            char msg[BUFFER_SIZE];
            queue_pop(queue, msg);
            
            parser(msg);
        }
        sleep_ms(1000);
    }
    return 0;
}