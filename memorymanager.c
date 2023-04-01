#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "memorymanager.h"

block_t *create_memory_block(uint16_t location, uint16_t size) {
    /*  Create a free memory block.
     */
    block_t *block;
    block = (block_t *)malloc(sizeof(*block));
    assert(block);
    block->status = FREE;
    block->location = location;
    block->size = size;
    return block;
}

list_t *mm_init(uint16_t size) {
    /*  Initialize the memory manager, which is a linked list of memory blocks.
        The `size` parameter is the total size of memory.
     */
    list_t *memory = create_empty_list();
    memory = append(memory, create_memory_block(0, size));
    return memory;
}

void *mm_malloc(list_t *memory, uint16_t size) {
    /*  Allocate memory of size `size` to the process.
        The memory manager will find the best fit block and allocate it.
        If there is no free block that can fit the process, return NULL.
    */
    // TO DO: implement this function
    return NULL;
}

void mm_free(list_t *memory, uint16_t location) {
    /*  Free the memory block at `location`.
        The memory manager will merge the block with its adjacent free blocks.
        If the block is already free, do nothing.
     */
    // TO DO: implement this function
}

/* =============================================================================
   Written by David Sha.
============================================================================= */
