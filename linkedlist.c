#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linkedlist.h"

list_t *create_empty_list() {
    /*  Creates an empty linked list.
     */
    list_t *list;
    list = (list_t *)malloc(sizeof(*list));
    assert(list);
    list->head = list->foot = NULL;
    return list;
}

list_t *create_list(node_t *head, node_t *foot) {
    /*  Create a linked list given the head and foot.
     */
    assert(head && foot);
    list_t *new = create_empty_list();
    new->head = head;
    new->foot = foot;
    return new;
}

int is_empty_list(list_t *list) {
    /*  Checks if list is empty.
     */
    assert(list);
    return list->head == NULL;
}

void free_list(list_t *list, void (*free_data)(void *data)) {
    /*  Free the list by freeing all nodes and its contents.
        Give a function pointer to free the data. If no function 
        pointer is given, the data will not be freed.
     */
    assert(list);
    node_t *curr, *prev;
    curr = list->head;
    while (curr) {
        prev = curr;
        curr = curr->next;
        if (free_data) {
            free_data(prev->data);
        }
        free(prev);
    }
    free(list);
}

list_t *prepend(list_t *list, void *data) {
    /*  Prepend to the list i.e. add to head of linked list.
     */
    assert(list && data);
    node_t *new;
    new = (node_t *)malloc(sizeof(*new));
    assert(new);
    new->data = data;
    new->next = list->head;
    list->head = new;
    if (list->foot == NULL) {
        /* this is the first insert into list */
        list->foot = new;
    }
    return list;
}

list_t *append(list_t *list, void *data) {
    /*  Append to the list i.e. add to foot of linked list.
     */
    assert(list && data);
    node_t *new;
    new = (node_t *)malloc(sizeof(*new));
    assert(new);
    new->data = data;
    new->next = NULL;
    if (list->foot == NULL) {
        /* this is the first insert into list */
        list->head = list->foot = new;
    } else {
        list->foot->next = new;
        list->foot = new;
    }
    return list;
}

int list_len(list_t *list) {
    /*  Get the linked list length.
     */
    assert(list);
    int len = 0;
    node_t *curr;
    curr = list->head;
    while (curr) {
        len++;
        curr = curr->next;
    }
    return len;
}

void *remove_data(list_t *list, void *data) {
    /*  Remove data from the list. If data is not in the list, return NULL.
     */
    assert(list && data);
    node_t *curr, *prev;
    curr = list->head;
    prev = NULL;
    while (curr) {
        if (curr->data == data) {
            if (prev == NULL) {
                /* removing the first node in the list */
                list->head = curr->next;
                if (list->foot == curr) {
                    /* also removing the last node in the list */
                    list->foot = NULL;
                }
            } else {
                /* removing a node in the middle or end of the list */
                prev->next = curr->next;
                if (list->foot == curr) {
                    /* also removing the last node in the list */
                    list->foot = prev;
                }
            }
            free(curr);
            return data;
        }
        prev = curr;
        curr = curr->next;
    }
    return NULL;
}

void *move_data(void *data, list_t *from, list_t *to) {
    /*  Move data from one list to another.
     */
    assert(data && from && to);
    void *removed = remove_data(from, data);
    if (removed) {
        append(to, removed);
    }
    return removed;
}

void *pop(list_t *list) {
    /*  Pop from the head of list.
     */
    assert(list);
    node_t *head = list->head;
    if (head) {
        list->head = head->next;
        if (list->foot == head) {
            /* also removing the last node in the list */
            list->foot = NULL;
        }
        void *data = head->data;
        free(head);
        return data;
    }
    return NULL;
}

void print_list(list_t *list, void (*print_data)(void *)) {
    /*  Print a linked list in the format [data1, data2, ...].
     */
    assert(list);
    printf("[");
    for (node_t *curr = list->head; curr; curr = curr->next) {
        print_data(curr->data);
        if (curr->next) {
            printf(", ");
        }
    }
    printf("]\n");
}

/* =============================================================================
   Written by David Sha.
   - Originally written for COMP20003 Assignment 2, 2022, altered to fit
     this project.
   - Implementation of linked list structs inspired by Artem Polyvyanyy from
     ass2-soln-2020.c.
   - Implementation of linked list functions inspired by Alistair Moffat
     from "Programming, Problem Solving, and Abstraction with C".
============================================================================= */
