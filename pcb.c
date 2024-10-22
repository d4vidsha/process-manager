/* =============================================================================
   pcb.c

   The implementation of process control blocks. This file contains the
   API required to manage a process control block. We use process control
   blocks to represent states of processes in the system. Multiple
   PCBs make up a process table or process queue, both of which are stored
   as linked lists.

   Author: David Sha
============================================================================= */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include "pcb.h"

pcb_t *create_pcb(char *line) {
    /*  Given a line from the input file, parse it and return a pcb_t
        struct.

        The line should be in the format:
        <arrival time> <name> <service time> <memory size>
    */
    pcb_t *pcb;
    pcb = (pcb_t *)malloc(sizeof(*pcb));
    assert(pcb);
    char *token = strtok(line, SEPARATOR);
    pcb->arrival_time = (uint32_t)strtoul(token, NULL, 10);
    token = strtok(NULL, SEPARATOR);
    char *name;
    name = (char *)malloc(strlen(token) + 1);
    assert(name);
    pcb->name = memcpy(name, token, strlen(token) + 1);
    token = strtok(NULL, SEPARATOR);
    pcb->service_time = (uint32_t)strtoul(token, NULL, 10);
    pcb->remaining_time = pcb->service_time;
    token = strtok(NULL, SEPARATOR);
    pcb->memory_size = (uint16_t)strtoul(token, NULL, 10);
    pcb->memory = NULL;
    pcb->process = NULL;
    pcb->state = NEW;
    pcb->termination_time = 0;
    return pcb;
}

void free_pcb(void *data) {
    /*  Free a pcb_t struct.
     */
    pcb_t *pcb = (pcb_t *)data;
    free(pcb->name);
    free(pcb);
}

void print_pcb(void *data) {
    /*  Print a pcb_t struct.
     */
    pcb_t *pcb = (pcb_t *)data;
    printf("%" PRIu32 " %s %" PRIu32 " %" PRIu16, pcb->arrival_time, pcb->name,
           pcb->service_time, pcb->memory_size);
}

process_t *initialise_process(pcb_t *pcb) {
    /*  Create a process_t struct and fork a process to run the process
        executable.
     */
    process_t *process;
    process = (process_t *)malloc(sizeof(*process));
    assert(process);
    pcb->process = process;

    // create pipes for communication with process
    if (pipe(process->fd) == FAILED) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(process->parent_fd) == FAILED) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // use fork to create a new process
    switch (process->pid = fork()) {
    case FAILED:
        perror("fork");
        exit(EXIT_FAILURE);

    case 0: // child process
        if (close(process->fd[1]) == FAILED) {
            perror("close");
            exit(EXIT_FAILURE);
        }
        if (close(process->parent_fd[0]) == FAILED) {
            perror("close");
            exit(EXIT_FAILURE);
        }

        // redirect stdin and stdout to pipes
        if (dup2(process->fd[0], STDIN_FILENO) == FAILED) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        if (dup2(process->parent_fd[1], STDOUT_FILENO) == FAILED) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        if (close(process->fd[0]) == FAILED) {
            perror("close");
            exit(EXIT_FAILURE);
        }
        if (close(process->parent_fd[1]) == FAILED) {
            perror("close");
            exit(EXIT_FAILURE);
        }

        // execute process executable
        char *cmd[] = {PROCESS_EXECUTABLE, pcb->name, NULL};
        if (execvp(cmd[0], cmd) == FAILED) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }

    default: // parent process
        if (close(process->fd[0]) == FAILED) {
            perror("close");
            exit(EXIT_FAILURE);
        }
        if (close(process->parent_fd[1]) == FAILED) {
            perror("close");
            exit(EXIT_FAILURE);
        }
        // all other file descriptors close from `terminate_process()`
    }
    return process;
}
