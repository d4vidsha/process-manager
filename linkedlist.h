#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_

/* #includes ================================================================ */
#include "main.h"

/* structures =============================================================== */
typedef struct node node_t;
struct node {
    pcb_t *pcb;
    node_t *next;
};

typedef struct list list_t;
struct list {
    node_t *head;
    node_t *foot;
};

/* function prototypes ====================================================== */
list_t *create_empty_list();
list_t *create_list(node_t *head, node_t *foot);
int is_empty_list(list_t *list);
void free_list(list_t *list);
list_t *prepend(list_t *list, pcb_t *pcb);
list_t *append(list_t *list, pcb_t *pcb);
int list_len(list_t *list);

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */
