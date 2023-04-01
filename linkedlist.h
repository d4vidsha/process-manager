#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_

/* structures =============================================================== */
typedef struct node node_t;
struct node {
    void *data;
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
void free_list(list_t *list, void (*free_data)(void *data));
list_t *prepend(list_t *list, void *data);
list_t *append(list_t *list, void *data);
int list_len(list_t *list);
void *remove_data(list_t *list, void *data);
void *move_data(void *data, list_t *from, list_t *to);
void *pop(list_t *list);
void print_list(list_t *list, void (*print_data)(void *));

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */
