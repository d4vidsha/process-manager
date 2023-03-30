#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "main.h"
#include "linkedlist.h"
#include "config.h"

const char *const SCHEDULERS[] = {"SJF", "RR", NULL};
const char *const MEMORY_METHODS[] = {"infinite", "best-fit", NULL};
const char *const QUANTUMS[] = {"1", "2", "3", NULL};

int main(int argc, char *argv[]) {

    // read in flags and arguments
    args_t *args = parse_args(argc, argv);
    assert(args->file != NULL);
    assert(args->scheduler != NULL);
    assert(args->memory != NULL);
    assert(args->quantum != NULL);

    // run simulation
    process_manager(args);

    free(args);
    return 0;
}

void process_manager(args_t *args) {
    /*  Given a struct of arguments that contains the file, scheduler,
        memory method, and quantum, run the simulation.
    */
    if (DEBUG) {
        printf("ACTION: Running simulation with the following arguments...\n");
        printf("File: %s\n", args->file);
        printf("Scheduler: %s\n", args->scheduler);
        printf("Memory: %s\n", args->memory);
        printf("Quantum: %s\n", args->quantum);
    }

    // read in file
    if (DEBUG) {
        printf("ACTION: Reading in file...\n");
    }
    FILE *fp = fopen(args->file, "r");
    assert(fp);

    // for each line in the file, parse it and add it to a linked list
    list_t *submitted_pcbs = create_empty_list();
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
        pcb_t *pcb = parse_pcb_line(line);
        append(submitted_pcbs, pcb);
    }

    // print submitted queue
    if (DEBUG) {
        printf("Number of processes in linked list: %d\n",
               list_len(submitted_pcbs));
        node_t *curr = submitted_pcbs->head;
        while (curr) {
            pcb_t *pcb = (pcb_t *)curr->data;
            printf("%s, arrive: %" PRIu32 "s, service: %" PRIu32
                   "s, mem: %" PRIu16 "MB, state: %d\n",
                   pcb->name, pcb->arrival_time, pcb->service_time,
                   pcb->memory_size, pcb->state);
            curr = curr->next;
        }
    }

    // free line if necessary
    if (line) {
        free(line);
    }

    // close file
    fclose(fp);

    list_t *input_queue = create_empty_list();
    list_t *ready_queue = create_empty_list();
    list_t *running_queue = create_empty_list();
    list_t *finished_queue = create_empty_list();
    int quantum = atoi(args->quantum);
    int simulation_time = 0;
    while (!(is_empty_list(submitted_pcbs) == TRUE &&
             is_empty_list(input_queue) == TRUE &&
             is_empty_list(ready_queue) == TRUE &&
             is_empty_list(running_queue) == TRUE)) {
        if (DEBUG) {
            printf("%d\n", simulation_time);
        }

        // if current running process (if any) has completed, terminate it and
        // deallocate its memory
        if (!is_empty_list(running_queue)) {
            pcb_t *pcb = (pcb_t *)running_queue->head->data;
            printf("TEST: %d\n", pcb->service_time);
            if (pcb->service_time <= 0) {
                pcb->state = FINISHED;
                move_data(pcb, running_queue, finished_queue);
                printf("%d,FINISHED,process_name=%s,proc_remaining=%d\n",
                       simulation_time, pcb->name,
                       list_len(ready_queue) + list_len(input_queue));
            }
        }

        // identify all processes that have been submitted since the last cycle
        // occurred and add them to the input queue in the order they appear
        // in the process file. A process is considered to have been submitted
        // to the system if its arrival time is less than or equal to the
        // current simulation time

        // update the state of each process in the system
        node_t *curr;
        curr = submitted_pcbs->head;
        while (curr) {
            pcb_t *pcb = (pcb_t *)curr->data;
            if (pcb->arrival_time <= simulation_time) {
                pcb->state = READY;
            }
            curr = curr->next;
        }

        // move processes to input queue if they have READY state
        curr = submitted_pcbs->head;
        node_t *next = NULL;
        while (curr) {
            pcb_t *pcb = (pcb_t *)curr->data;
            next = curr->next;
            if (pcb->state == READY) {
                move_data(pcb, submitted_pcbs, input_queue);
            }
            curr = next;
        }

        // move processes from the input queue to the ready queue upon
        // successful memory allocation. The ready queue holds all
        // processes that are ready to run
        // memory allocation can be done in one of two ways:
        // infinite or best-fit

        curr = input_queue->head;
        next = NULL;
        while (curr) {
            pcb_t *pcb = (pcb_t *)curr->data;
            next = curr->next;
            if (strcmp(args->memory, "infinite") == 0) {
                move_data(pcb, input_queue, ready_queue);
            } else if (strcmp(args->memory, "best-fit") == 0) {
                // TODO: implement best-fit
            }
            curr = next;
        }

        // determine the process (if any) that will run in this cycle.
        // Depending on the scheduling algorithm, this may be the process
        // that has been previously running, or a ready process which
        // has not been previously running
        pcb_t *running = NULL;
        if (strcmp(args->scheduler, "SJF") == 0) {
            // TODO: implement shortest job first
        } else if (strcmp(args->scheduler, "RR") == 0) {
            // TODO: implement round robin
            running = pop(ready_queue);
            if (running) {
                running->state = RUNNING;
                printf("%d,RUNNING,process_name=%s,remaining_time=%d\n",
                       simulation_time, running->name, running->service_time);
                move_data(running, ready_queue, running_queue);
            }
        }

        simulation_time += quantum;

        // print submitted queue
        if (DEBUG) {
            printf("Submitted queue: ");
            curr = submitted_pcbs->head;
            while (curr) {
                pcb_t *pcb = (pcb_t *)curr->data;
                assert(pcb->state == NEW);
                printf("%s ", pcb->name);
                curr = curr->next;
            }
            printf("\n");

            // print input queue
            printf("    Input queue: ");
            curr = input_queue->head;
            while (curr) {
                pcb_t *pcb = (pcb_t *)curr->data;
                assert(pcb->state == READY);
                printf("%s ", pcb->name);
                curr = curr->next;
            }
            printf("\n");

            // print ready queue
            printf("    Ready queue: ");
            curr = ready_queue->head;
            while (curr) {
                pcb_t *pcb = (pcb_t *)curr->data;
                assert(pcb->state == READY);
                printf("%s ", pcb->name);
                curr = curr->next;
            }
            printf("\n");

            // print running queue
            printf("  Running queue: ");
            curr = running_queue->head;
            while (curr) {
                pcb_t *pcb = (pcb_t *)curr->data;
                assert(pcb->state == RUNNING);
                printf("%s ", pcb->name);
                curr = curr->next;
            }
            printf("\n");

            // print finished queue
            printf(" Finished queue: ");
            curr = finished_queue->head;
            while (curr) {
                pcb_t *pcb = (pcb_t *)curr->data;
                assert(pcb->state == FINISHED);
                printf("%s ", pcb->name);
                curr = curr->next;
            }
            printf("\n");
        }
    }

    // free linked lists
    free_list(submitted_pcbs, free_pcb);
    free_list(input_queue, free_pcb);
    free_list(ready_queue, free_pcb);
    free_list(running_queue, free_pcb);
    free_list(finished_queue, free_pcb);
}

char *read_flag(char *flag, const char *const *valid_args, int argc,
                char *argv[]) {
    /*  Given a flag and a location in the argument list, return the
        argument following the flag provided that it is in the set of
        valid arguments. Otherwise, return NULL.
    */
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], flag) == 0) {

            // if valid_args is NULL, then we don't care about the argument
            if (valid_args == NULL) {
                return argv[i + 1];
            } else {

                // check if the argument is valid
                int j = 0;
                while (valid_args[j] != NULL) {
                    if (strcmp(argv[i + 1], valid_args[j]) == 0) {
                        return argv[i + 1];
                    }
                    j++;
                }

                // if we get here, the argument was not valid
                printf("Invalid argument for flag %s. Must be one of: ", flag);
                j = 0;
                while (valid_args[j] != NULL) {
                    printf("%s ", valid_args[j]);
                    j++;
                }
                printf("\n");
                exit(1);
            }
        }
    }
    return NULL;
}

args_t *parse_args(int argc, char *argv[]) {
    /*  Given a list of arguments, parse them and return all flags and
        arguments in a struct.
    */
    args_t *args;
    args = (args_t *)malloc(sizeof(*args));
    args->file = read_flag("-f", NULL, argc, argv);
    args->scheduler = read_flag("-s", SCHEDULERS, argc, argv);
    args->memory = read_flag("-m", MEMORY_METHODS, argc, argv);
    args->quantum = read_flag("-q", QUANTUMS, argc, argv);
    return args;
}

pcb_t *parse_pcb_line(char *line) {
    /*  Given a line from the input file, parse it and return a pcb_t
        struct.

        The line should be in the format:
        <arrival time> <name> <service time> <memory size>
    */
    pcb_t *pcb;
    pcb = (pcb_t *)malloc(sizeof(*pcb));
    char *token = strtok(line, SEPARATOR);
    pcb->arrival_time = (uint32_t)strtoul(token, NULL, 10);
    token = strtok(NULL, SEPARATOR);
    pcb->name = memcpy(malloc(strlen(token) + 1), token, strlen(token) + 1);
    token = strtok(NULL, SEPARATOR);
    pcb->service_time = (uint32_t)strtoul(token, NULL, 10);
    token = strtok(NULL, SEPARATOR);
    pcb->memory_size = (uint16_t)strtoul(token, NULL, 10);
    pcb->state = NEW;
    return pcb;
}

void free_pcb(void *data) {
    /*  Free a pcb_t struct.
     */
    pcb_t *pcb = (pcb_t *)data;
    free(pcb->name);
    free(pcb);
}

/* =============================================================================
   Written by David Sha.
============================================================================= */
