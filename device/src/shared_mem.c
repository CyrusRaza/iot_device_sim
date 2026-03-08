#include "shared_mem.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
//#include <stdatomic.h>



shared_mem* queue_create()
{
    shared_mem* queue = malloc(sizeof(shared_mem));
    if (!queue)
    {
        return NULL;
    }

    for (int i = 0; i < BUFFER_SIZE; i++) 
    {
        queue->char_arr[i] = NULL;
    }
    atomic_init(&(queue->read_ind), 0);
    atomic_init(&(queue->write_ind), 0);
    return queue;
}


int queue_push(struct shared_mem* self, char* data, int len)
{
    if (!queue_is_full(self))
    {
        int idx = self->write_ind % BUFFER_SIZE;
        self->char_arr[idx] = malloc(len+1);
        if (!self->char_arr[idx])
        {
            return 1;
        }
        strncpy(self->char_arr[idx], data, len);
        self->char_arr[idx][len] = '\0';
        printf("write ind %i\n", self->write_ind);
        atomic_fetch_add(&(self->write_ind), 1);
        return 0;
    }
    else
    {
        printf("buffer full\n");
        return 1;
    }
}

int queue_pop(struct shared_mem* self, char* popped_data)
{
    if (!queue_is_empty(self))
    {
        int idx = self->read_ind % BUFFER_SIZE;
        strcpy(popped_data, self->char_arr[idx]);
        free(self->char_arr[idx]);
        self->char_arr[idx] = NULL;
        atomic_fetch_add(&self->read_ind, 1);
        
        return 0;
    }
    else
    {
        printf("buffer empty\n");
        return 1;
    }
}

bool queue_is_full(struct shared_mem* self)
{
    return ((self->write_ind - self->read_ind) >= BUFFER_SIZE);
}

bool queue_is_empty(struct shared_mem* self)
{
    //printf("write : %i, read : %i\n", self->write_ind, self->read_ind);
    return ((self->write_ind) == ((self->read_ind)));
}

int queue_free_spaces(struct shared_mem* self)
{
    return (BUFFER_SIZE - ((self->write_ind) - (self->read_ind)));
}

int queue_filled_spaces(struct shared_mem* self)
{
    return ((self->write_ind) - (self->read_ind));
}

void queue_destroy(shared_mem* queue)
{
    char temp[BUFFER_SIZE+1];
    while (queue_pop(queue, temp) !=1){}
    free(queue);
}