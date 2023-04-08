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

    // create pipes for communication
    if (pipe(process->to_process) == -1) {
        perror("pipe");
        exit(1);
    }
    if (pipe(process->to_manager) == -1) {
        perror("pipe");
        exit(1);
    }

    // use fork to create a new process
    switch (process->pid = fork()) {
    case -1:
        // error
        perror("fork");
        exit(1);

    case 0:
        // child process
        close(process->to_process[1]);
        close(process->to_manager[0]);

        // redirect stdin and stdout to pipes
        dup2(process->to_process[0], STDIN_FILENO);
        dup2(process->to_manager[1], STDOUT_FILENO);

        // execute process executable
        char *cmd[] = {"./process", pcb->name, NULL};
        execvp(cmd[0], cmd);

    default:
        // parent process
        close(process->to_process[0]);
        close(process->to_manager[1]);
    }
    return process;
}

/* =============================================================================
   Written by David Sha.
============================================================================= */