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

/* function prototypes ====================================================== */
uint32_t average_turnaround_time(list_t *finished_queue);
float max_time_overhead(list_t *finished_queue);
float average_time_overhead(list_t *finished_queue);
char *read_flag(char *flag, const char *const *valid_args, int argc,
                char *argv[]);
args_t *parse_args(int argc, char *argv[]);
void process_manager(args_t *args);
void run_cycles(list_t *submitted_pcbs, args_t *args);
void big_endian(uint32_t value, char *big_endian);

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */
