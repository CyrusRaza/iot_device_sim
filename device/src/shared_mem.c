#include "shared_mem.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
//#include <stdatomic.h>



shared_mem* queue_create()
{
    shared_mem* queue = malloc(sizeof(shared_mem));
    *queue = (shared_mem){
        .char_arr = "",
    };
    atomic_init(&(queue->read_ind), 0);
    atomic_init(&(queue->write_ind), 0);
    return queue;
}


int queue_push(struct shared_mem* self, char* data, int len)
{
    if (!queue_is_full(self))
    {
        self->char_arr[self->write_ind % BUFFER_SIZE] = malloc(len+1);
        strncpy(self->char_arr[self->write_ind % BUFFER_SIZE], data, len);
        self->char_arr[self->write_ind % BUFFER_SIZE][len] = '\0';
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
        strcpy(popped_data, self->char_arr[self->read_ind % BUFFER_SIZE]);
        free(self->char_arr[self->read_ind % BUFFER_SIZE]);
        memset(self->char_arr,0, BUFFER_SIZE);
        printf("read_int %i\n", self->read_ind);
        atomic_fetch_add(&self->read_ind, 1);
        printf("read_int %i\n", self->read_ind);
        
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
    if ((self->write_ind % BUFFER_SIZE) == ((self->read_ind % BUFFER_SIZE) - 1))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool queue_is_empty(struct shared_mem* self)
{
    //printf("write : %i, read : %i\n", self->write_ind, self->read_ind);
    if ((self->write_ind) == ((self->read_ind)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

int queue_free_spaces(struct shared_mem* self)
{
    return (BUFFER_SIZE - ((self->write_ind % BUFFER_SIZE) - (self->read_ind % BUFFER_SIZE)));
}

int queue_filled_spaces(struct shared_mem* self)
{
    return ((self->write_ind % BUFFER_SIZE) - (self->read_ind % BUFFER_SIZE));
}

void queue_destroy(shared_mem* queue)
{
    int i = 0;
    char* temp;
    while (i!=1)
    {
        i = queue_pop(queue, temp);
    }
    free(queue);
}