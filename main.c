#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "main.h"

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
    if (line) {
        free(line);
    }
    fclose(fp);
    if (DEBUG) {
        printf("ACTION: Printing submitted queue...\n");
        print_list(submitted_pcbs, print_pcb);
    }

    // run simulation given the processes in the submitted queue and the args
    run_cycles(submitted_pcbs, args);
    free_list(submitted_pcbs, free_pcb);
}

void run_cycles(list_t *process_table, args_t *args) {
    /*  This function runs the simulation for the given list of submitted
        processes and the given arguments.
    */
    list_t *submitted_queue = create_empty_list();
    list_t *input_queue = create_empty_list();
    list_t *ready_queue = create_empty_list();
    list_t *running_queue = create_empty_list();
    list_t *finished_queue = create_empty_list();
    int simulation_time = 0;
    int quantum = atoi(args->quantum);
    int num_processes = list_len(process_table);

    // copy processes from process table to submitted queue
    for (node_t *curr = process_table->head; curr != NULL; curr = curr->next) {
        pcb_t *pcb = (pcb_t *)curr->data;
        append(submitted_queue, pcb);
    }

    // on each cycle
    while (TRUE) {
        if (DEBUG) {
            printf("%d\n", simulation_time);
            printf("submitted: ");
            print_list(submitted_queue, print_pcb);
            printf("    input: ");
            print_list(input_queue, print_pcb);
            printf("    ready: ");
            print_list(ready_queue, print_pcb);
            printf("  running: ");
            print_list(running_queue, print_pcb);
            printf(" finished: ");
            print_list(finished_queue, print_pcb);
        }

        // if current running process (if any) has completed, terminate it and
        // deallocate its memory
        while (TRUE) {
            node_t *curr = running_queue->head;

            // if there are no more running processes, break
            if (curr == NULL) {
                break;
            }

            // assuming there are more running processes, check if they
            // should be terminated
            pcb_t *pcb = (pcb_t *)curr->data;

            // decrement service time by the quantum. if service time becomes
            // negative, set it to 0
            if (pcb->service_time < quantum) {
                pcb->service_time = 0;
            } else {
                pcb->service_time -= quantum;
            }

            if (pcb->service_time == 0) {
                if (DEBUG) {
                    printf("ACTION: Terminating process %s\n", pcb->name);
                }
                printf("%d,FINISHED,process_name=%s,proc_remaining=%d\n",
                       simulation_time, pcb->name,
                       list_len(input_queue) + list_len(ready_queue));
                move_data(pcb, running_queue, finished_queue);
            } else {
                break;
            }
        }

        // identify all processes that have been submitted since the last cycle
        // occurred and add them to the input queue in the order they appear
        // in the process file. A process is considered to have been submitted
        // to the system if its arrival time is less than or equal to the
        // current simulation time
        while (TRUE) {
            node_t *curr = submitted_queue->head;

            // if there are no more submittable processes, break
            if (curr == NULL) {
                break;
            }

            // assuming there are more submittable processes, check if they
            // should be added to the input queue
            pcb_t *pcb = (pcb_t *)curr->data;
            if (pcb->arrival_time <= simulation_time) {
                if (DEBUG) {
                    printf("ACTION: Adding process %s to input queue\n",
                           pcb->name);
                }
                move_data(pcb, submitted_queue, input_queue);
            } else {
                // this assumes that the processes are sorted by arrival time
                break;
            }
        }

        // move processes from the input queue to the ready queue upon
        // successful memory allocation. The ready queue holds all
        // processes that are ready to run
        // memory allocation can be done in one of two ways:
        // infinite or best-fit
        if (strcmp(args->memory, "infinite") == 0) {
            while (TRUE) {
                node_t *curr = input_queue->head;

                // if there are no more input processes, break
                if (curr == NULL) {
                    break;
                }

                // assuming there are more input processes, move them
                // directly to the ready queue
                pcb_t *pcb = (pcb_t *)curr->data;
                if (DEBUG) {
                    printf("ACTION: Adding process %s to ready queue\n",
                           pcb->name);
                }
                move_data(pcb, input_queue, ready_queue);
            }
        } else if (strcmp(args->memory, "best-fit") == 0) {
            // TO DO: implement best-fit memory allocation
        }

        // determine the process (if any) that will run in this cycle.
        // Depending on the scheduling algorithm, this may be the process
        // that has been previously running, or a ready process which
        // has not been previously running
        if (strcmp(args->scheduler, "SJF") == 0) {
            // find the process with the shortest service time
            // if two processes have the same service time, choose the
            // process that arrived first
            // if two processes have the same service time and arrival
            // time, choose the process whose name comes first
            // lexicographically
            if (running_queue->head == NULL) {
                // only add a process to running queue if there is no
                // running process
                node_t *curr = ready_queue->head;
                node_t *min = curr;
                while (curr) {
                    pcb_t *pcb = (pcb_t *)curr->data;
                    pcb_t *min_pcb = (pcb_t *)min->data;
                    if (pcb->service_time < min_pcb->service_time) {
                        min = curr;
                    } else if (pcb->service_time == min_pcb->service_time) {
                        if (pcb->arrival_time < min_pcb->arrival_time) {
                            min = curr;
                        } else if (pcb->arrival_time == min_pcb->arrival_time) {
                            if (strcmp(pcb->name, min_pcb->name) < 0) {
                                min = curr;
                            }
                        }
                    }
                    curr = curr->next;
                }

                // provided there exists a process with the shortest service
                // time, add it to the running queue
                if (min) {
                    pcb_t *pcb = (pcb_t *)min->data;
                    if (DEBUG) {
                        printf("ACTION: Adding process %s to running queue\n",
                               pcb->name);
                    }
                    printf("%d,RUNNING,process_name=%s,remaining_time=%" PRIu32
                           "\n",
                           simulation_time, pcb->name, pcb->service_time);
                    move_data(min->data, ready_queue, running_queue);
                }
            }
        } else if (strcmp(args->scheduler, "RR") == 0) {
            // if there are no processes in the ready queue, the process
            // that is currently running continues to run
            if (ready_queue->head == NULL) {
                // do nothing, let the process continue to run
            } else {
                // if there is a process currently running, suspend and add it
                // to the end of the ready queue, assuming only one running
                // process exists at a time
                if (running_queue->head != NULL) {
                    pcb_t *pcb = (pcb_t *)running_queue->head->data;
                    if (DEBUG) {
                        printf("ACTION: Suspending process %s\n", pcb->name);
                    }
                    move_data(pcb, running_queue, ready_queue);
                }

                // the process at the head of ready queue is chosen to run for
                // one quantum
                pcb_t *pcb = (pcb_t *)ready_queue->head->data;
                if (DEBUG) {
                    printf("ACTION: Adding process %s to running queue\n",
                           pcb->name);
                }
                printf("%d,RUNNING,process_name=%s,remaining_time=%" PRIu32
                       "\n",
                       simulation_time, pcb->name, pcb->service_time);
                move_data(pcb, ready_queue, running_queue);
            }
        }

        // when the finished queue contains all processes, the process manager
        // can terminate
        if (list_len(finished_queue) == num_processes) {
            break;
        }

        simulation_time += quantum;
    }

    // free linked lists
    free_list(submitted_queue, NULL);
    free_list(input_queue, NULL);
    free_list(ready_queue, NULL);
    free_list(running_queue, NULL);
    free_list(finished_queue, NULL);
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

/* =============================================================================
   Written by David Sha.
============================================================================= */
