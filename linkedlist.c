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

void free_list(list_t *list) {
    /*  Free the list by freeing all nodes and its contents.
    */
    assert(list);
    node_t *curr, *prev;
    curr = list->head;
    while (curr) {
        prev = curr;
        curr = curr->next;
        free(prev);
    }
    free(list);
}

list_t *prepend(list_t *list, pcb_t *pcb) {
    /*  Prepend to the list i.e. add to head of linked list.
    */
    assert(list && pcb);
    node_t *new;
    new = (node_t *)malloc(sizeof(*new));
    assert(new);
    new->pcb = pcb;
    new->next = list->head;
    list->head = new;
    if (list->foot == NULL) {
        /* this is the first insert into list */
        list->foot = new;
    }
    return list;
}

list_t *append(list_t *list, pcb_t *pcb) {
    /*  Append to the list i.e. add to foot of linked list.
    */
    assert(list && pcb);
    node_t *new;
    new = (node_t *)malloc(sizeof(*new));
    assert(new);
    new->pcb = pcb;
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

/* =============================================================================
   Written by David Sha.
   - Originally written for COMP20003 Assignment 2, 2022, altered to fit
     this project.
   - Implementation of linked list structs inspired by Artem Polyvyanyy from
     ass2-soln-2020.c.
   - Implementation of linked list functions inspired by Alistair Moffat
     from "Programming, Problem Solving, and Abstraction with C".
============================================================================= */