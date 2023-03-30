#ifndef _MAIN_H_
#define _MAIN_H_

/* #includes ================================================================ */
#include <stdint.h>

/* constants ================================================================ */
#define SEPARATOR " "
#define DEBUG 1

/* structures =============================================================== */
typedef struct args {
    char *file;
    char *scheduler;
    char *memory;
    char *quantum;
} args_t;

typedef struct process_control_block {
    char *name;
    uint32_t arrival_time;
    uint32_t service_time;
    uint16_t memory_size;
    enum state {NEW, READY, RUNNING, FINISHED} state;
} pcb_t;

/* function prototypes ====================================================== */
char *read_flag(char *flag, const char *const *valid_args, int argc,
                char *argv[]);
args_t parse_args(int argc, char *argv[]);
void process_manager(args_t args);
pcb_t *parse_pcb_line(char *line);
void cycle_tasks();
void free_pcb(void *data);

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */
