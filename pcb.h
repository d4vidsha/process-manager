#ifndef _PCB_H_
#define _PCB_H_

/* #includes ================================================================ */
#include <stdint.h>
#include "memorymanager.h"
#include "process-api.h"

/* structures =============================================================== */
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
pcb_t *create_pcb(char *line);
void free_pcb(void *data);
void print_pcb(void *data);
process_t *initialise_process(pcb_t *pcb);

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */
