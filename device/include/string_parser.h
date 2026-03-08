

#define NUM_COMMANDS 5

extern const char* command_list[NUM_COMMANDS];
extern int command_vals[NUM_COMMANDS];

#ifndef SHARED_LOCK_H
#define SHARED_LOCK_H

#include <threads.h>

extern mtx_t command_vals_mutex;

#endif

void sleep_ms(long ms);
void print_commands();
int string_parser(void* obj);
void init_shared_lock(void);

int check_key(char *key);
void remove_spaces(char *restrict str_trimmed, const char *restrict str_untrimmed);
void parser(char *msg);
void change_val(int val, int index);