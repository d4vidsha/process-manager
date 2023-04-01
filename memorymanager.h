#ifndef _MEMORYMANAGER_H_
#define _MEMORYMANAGER_H_

/* #includes ================================================================ */
#include <stdint.h>

/* structures =============================================================== */
typedef struct block {
    enum {FREE, ALLOCATED} status;
    uint16_t location;
    uint16_t size;
} block_t;

/* function prototypes ====================================================== */
void *mm_malloc(uint16_t size);
void mm_free(void *ptr);

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */
