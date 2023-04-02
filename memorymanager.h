#ifndef _MEMORYMANAGER_H_
#define _MEMORYMANAGER_H_

/* #includes ================================================================ */
#include <stdint.h>
#include "linkedlist.h"
#include "config.h"

/* structures =============================================================== */
typedef struct block {
    enum { FREE, ALLOCATED } status;
    uint16_t location;
    uint16_t size;
} block_t;

/* function prototypes ====================================================== */
block_t *create_memory_block(int status, uint16_t location, uint16_t size);
list_t *mm_init(uint16_t size);
block_t *mm_malloc(list_t *memory, uint16_t size);
void mm_free(list_t *memory, block_t *block);
void print_block(void *data);

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */
