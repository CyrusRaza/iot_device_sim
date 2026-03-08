#pragma once
#include <stdbool.h>
#include <stdatomic.h>
#define BUFFER_SIZE 20


typedef struct shared_mem shared_mem;

shared_mem* queue_create();
void queue_destroy(shared_mem* queue);
int queue_push(struct shared_mem* self, char* data, int len);
int queue_pop(struct shared_mem* self, char* popped_data);
bool queue_is_full(struct shared_mem* self);
bool queue_is_empty(struct shared_mem* self);
int queue_free_spaces(struct shared_mem* self);
int queue_filled_spaces(struct shared_mem* self);


typedef struct shared_mem
{
    char* char_arr[BUFFER_SIZE];
    atomic_int read_ind;
    atomic_int write_ind;
}shared_mem;

