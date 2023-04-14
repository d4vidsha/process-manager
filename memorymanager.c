/* =============================================================================
   memorymanager.c

   The implementation of the memory manager. This file contains the
   API required to manage the memory. The memory manager is implemented
   as a linked list of memory blocks.

   Author: David Sha
============================================================================= */
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

block_t *mm_malloc(list_t *memory, uint16_t size) {
    /*  Allocate memory of size `size` to the process.
        The memory manager will find the best fit block and allocate it.
        If there is no free block that can fit the process, return NULL.

        If there are two or more best fit blocks, the memory manager will
        allocate the block with the smallest address, i.e. leftmost block.
    */
    // find the best fit block
    block_t *min = NULL;
    for (node_t *curr = memory->head; curr; curr = curr->next) {
        block_t *block = (block_t *)curr->data;
        if (block->status == FREE && block->size >= size &&
            (!min || block->size < min->size)) {
            min = block;
        }
    }

    // split the block if not perfect fit and allocate
    if (min) {
        assert(min->status == FREE);
        if (size < min->size) {
            block_t *new = create_memory_block(ALLOCATED, min->location, size);
            min->size -= size;
            min->location += size;
            // get the node of the best fit block
            node_t *min_node = (node_t *)find_node(memory, min, cmp_addr);
            insert_prev(memory, min_node, new);
            return new;
        } else if (size == min->size) {
            // memory block is exactly the size of the process
            min->status = ALLOCATED;
            return min;
        } else {
            // memory block is smaller than the process, this should
            // never happen
            return NULL;
        }
    }

    return NULL;
}

void mm_free(list_t *memory, block_t *block) {
    /*  Free the memory block.
        The memory manager will merge the block with its adjacent free blocks.
        If the block is already free, do nothing.
     */

    // check if the block is already free
    if (block->status == FREE) {
        return;
    }

    for (node_t *curr = memory->head; curr; curr = curr->next) {
        block_t *curr_block = (block_t *)curr->data;

        if (cmp_addr(curr_block, block) == 0) {
            // the block is found

            assert(curr_block->status == ALLOCATED);
            curr_block->status = FREE;

            // merge next block into current block
            if (curr->next) {
                block_t *next = (block_t *)curr->next->data;
                if (next->status == FREE) {
                    curr_block->size += next->size;
                    remove_node(memory, curr->next);
                    free(next);
                }
            }

            // merge current block into previous block
            if (curr->prev) {
                block_t *prev = (block_t *)curr->prev->data;
                if (prev->status == FREE) {
                    prev->size += curr_block->size;
                    remove_node(memory, curr);
                    free(curr_block);
                }
            }

            break;
        }
    }
}

void print_block(void *data) {
    /*  Print the block.
     */
    block_t *block = (block_t *)data;
    printf("%s-%d-%d", block->status == FREE ? "FREED" : "ALLOC",
           block->location, block->size);
}
