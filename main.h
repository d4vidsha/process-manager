#ifndef _MAIN_H_
#define _MAIN_H_

/* #includes ================================================================ */
#include <stdint.h>
#include "config.h"
#include "linkedlist.h"
#include "memorymanager.h"

/* structures =============================================================== */
typedef struct arguments {
    char *file;
    char *scheduler;
    char *memory;
    char *quantum;
} args_t;

typedef struct process {
    pid_t pid;
    int to_process[2];
    int to_manager[2];
} process_t;

typedef struct process_control_block {
    char *name;
    enum { NEW, READY, RUNNING, SUSPENDED, TERMINATED } state;
    uint32_t arrival_time;
    uint32_t termination_time;
    uint32_t service_time;
    uint32_t remaining_time;
    uint16_t memory_size;
    block_t *memory;
    process_t *process;
} pcb_t;

/* function prototypes ====================================================== */
uint32_t average_turnaround_time(list_t *finished_queue);
float max_time_overhead(list_t *finished_queue);
float average_time_overhead(list_t *finished_queue);
char *read_flag(char *flag, const char *const *valid_args, int argc,
                char *argv[]);
args_t *parse_args(int argc, char *argv[]);
void process_manager(args_t *args);
pcb_t *parse_pcb_line(char *line);
void free_pcb(void *data);
void print_pcb(void *data);
void run_cycles(list_t *submitted_pcbs, args_t *args);
void convert_to_big_endian(uint32_t value, char *big_endian);
process_t *initialise_process(pcb_t *pcb);
void free_process(void *data);
void print_process(void *data);
void send_message(process_t *process, char *message);
void receive_message(process_t *process, char *message, int length);
void check_process(process_t *process, char *simulation_time);
void start_process(process_t *process, char *simulation_time);
void suspend_process(process_t *process, char *simulation_time);
void resume_process(process_t *process, char *simulation_time);
char *terminate_process(process_t *process, char *simulation_time);

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */
