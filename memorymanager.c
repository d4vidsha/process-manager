#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "memorymanager.h"

block_t *create_memory_block(int status, uint16_t location, uint16_t size) {
    /*  Create a free memory block.
     */
    block_t *block;
    block = (block_t *)malloc(sizeof(*block));
    assert(block);
    block->status = status;
    block->location = location;
    block->size = size;
    return block;
}

list_t *mm_init(uint16_t size) {
    /*  Initialize the memory manager, which is a linked list of memory blocks.
        The `size` parameter is the total size of memory.
     */
    list_t *memory = create_empty_list();
    memory = append(memory, create_memory_block(FREE, 0, size));
    return memory;
}

list_t *mm_malloc(list_t *memory, uint16_t size) {
    /*  Allocate memory of size `size` to the process.
        The memory manager will find the best fit block and allocate it.
        If there is no free block that can fit the process, return NULL.
    */
    // find the best fit block
    block_t *min = NULL;
    node_t *min_node = NULL;
    for (node_t *curr = memory->head; curr; curr = curr->next) {
        block_t *block = min = (block_t *)curr->data;
        if (block->status == FREE && block->size >= size &&
            block->size < min->size && block->location < min->location) {
            min = block;
            min_node = curr;
        }
    }

    // split the block if possible and allocate it
    if (min) {
        if (size < min->size) {
            block_t *new = create_memory_block(ALLOCATED, min->location, size);
            min->size -= size;
            min->location += size;
            return insert_prev(memory, min_node, new);
        } else if (size == min->size) {
            // memory block is exactly the size of the process
            min->status = ALLOCATED;
            return memory;
        } else {
            // memory block is smaller than the process, this should
            // never happen
            return NULL;
        }
    }

    return NULL;
}

void mm_free(list_t *memory, uint16_t location) {
    /*  Free the memory block at `location`.
        The memory manager will merge the block with its adjacent free blocks.
        If the block is already free, do nothing.
     */
    for (node_t *curr = memory->head; curr; curr = curr->next) {
        block_t *block = (block_t *)curr->data;

        if (block->location == location) {
        
            // the block is found at location
            if (block->status == ALLOCATED) {
                block->status = FREE;
        
                // merge with previous block
                if (curr->prev) {
                    block_t *prev = (block_t *)curr->prev->data;
                    if (prev->status == FREE) {
                        prev->size += block->size;
                        memory = remove_node(memory, curr);
                        free(block);
                    }
                }
        
                // merge with next block
                if (curr->next) {
                    block_t *next = (block_t *)curr->next->data;
                    if (next->status == FREE) {
                        block->size += next->size;
                        memory = remove_node(memory, curr->next);
                        free(next);
                    }
                }
            } else {
                // block is already free, do nothing
            }
            break;
        }
    }
}

/* =============================================================================
   Written by David Sha.
============================================================================= */
