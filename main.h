#ifndef _MAIN_H_
#define _MAIN_H_

/* #includes ================================================================ */
#include <stdint.h>
#include "config.h"
#include "pcb.h"
#include "linkedlist.h"
#include "memorymanager.h"
#include "process-api.h"

/* structures =============================================================== */
typedef struct arguments {
    char *file;
    char *scheduler;
    char *memory;
    char *quantum;
} args_t;

typedef struct cycle {
    uint32_t quantum;
    uint32_t simulation_time;
    char *big_endian;
    list_t *memory;
    list_t *submitted_queue;
    list_t *input_queue;
    list_t *ready_queue;
    list_t *running_queue;
    list_t *finished_queue;
} cycle_t;

/* function prototypes ====================================================== */
void process_manager(args_t *args);
void run_cycles(list_t *process_table, args_t *args);
void run_cycle(cycle_t *cycle, args_t *args);
cycle_t *create_cycle(uint32_t quantum);
void free_cycle(cycle_t *cycle);
void print_performance_statistics(cycle_t *cycle);
uint32_t average_turnaround_time(list_t *finished_queue);
float max_time_overhead(list_t *finished_queue);
float average_time_overhead(list_t *finished_queue);
char *read_flag(char *flag, const char *const *valid_args, int argc,
                char *argv[]);
args_t *parse_args(int argc, char *argv[]);
void big_endian(uint32_t value, char *big_endian);

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */
