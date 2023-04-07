#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
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
        pcb_t *pcb = create_pcb(line);
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
    list_t *memory = mm_init(MAX_MEMORY);
    list_t *submitted_queue = create_empty_list();
    list_t *input_queue = create_empty_list();
    list_t *ready_queue = create_empty_list();
    list_t *running_queue = create_empty_list();
    list_t *finished_queue = create_empty_list();
    uint32_t simulation_time = 0;
    uint32_t quantum = atoi(args->quantum);
    int num_processes = list_len(process_table);
    char *big_endian_simulation_time = calloc(4, sizeof(char));
    assert(big_endian_simulation_time);

    // big endian byte order for simulation time
    convert_to_big_endian(simulation_time, big_endian_simulation_time);
    if (DEBUG) {
        printf("INFO: Big Endian simulation time: %02x %02x %02x %02x\n",
               (uint8_t)big_endian_simulation_time[0],
               (uint8_t)big_endian_simulation_time[1],
               (uint8_t)big_endian_simulation_time[2],
               (uint8_t)big_endian_simulation_time[3]);
    }

    // copy processes from process table to submitted queue
    for (node_t *curr = process_table->head; curr != NULL; curr = curr->next) {
        pcb_t *pcb = (pcb_t *)curr->data;
        append(submitted_queue, pcb);
    }

    // on each cycle
    while (TRUE) {
        if (DEBUG) {
            printf("%" PRIu32 "\n", simulation_time);
            printf("   memory: ");
            print_list(memory, print_block);
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

            // decrement remaining time by the quantum. if remaining time
            // becomes negative, set it to 0
            if (pcb->remaining_time < quantum) {
                pcb->remaining_time = 0;
            } else {
                pcb->remaining_time -= quantum;
            }

            if (pcb->remaining_time == 0) {
                if (DEBUG) {
                    printf("ACTION: Terminating process %s\n", pcb->name);
                }
                if (strcmp(args->memory, "best-fit") == 0) {
                    mm_free(memory, pcb->memory);
                    pcb->memory = NULL;
                    if (DEBUG) {
                        print_list(memory, print_block);
                    }
                }
                printf("%" PRIu32
                       ",FINISHED,process_name=%s,proc_remaining=%d\n",
                       simulation_time, pcb->name,
                       list_len(input_queue) + list_len(ready_queue));
                move_data(pcb, running_queue, finished_queue);
                pcb->state = TERMINATED;
                pcb->termination_time = simulation_time;
                // task4: terminate process
                convert_to_big_endian(simulation_time,
                                      big_endian_simulation_time);
                char *sha256 =
                    terminate_process(pcb->process, big_endian_simulation_time);
                printf("%" PRIu32 ",FINISHED-PROCESS,process_name=%s,sha=%s\n",
                       simulation_time, pcb->name, sha256);
                free(sha256);
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
                pcb->state = NEW;
                // task4: initialise process
                initialise_process(pcb);
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
                pcb->state = READY;
            }
        } else if (strcmp(args->memory, "best-fit") == 0) {

            // copy input queue to a temporary queue
            list_t *temp_queue = create_empty_list();
            for (node_t *curr = input_queue->head; curr != NULL;
                 curr = curr->next) {
                pcb_t *pcb = (pcb_t *)curr->data;
                append(temp_queue, pcb);
            }

            // try to allocate memory for each process in the input queue
            for (node_t *curr = temp_queue->head; curr != NULL;
                 curr = curr->next) {
                pcb_t *pcb = (pcb_t *)curr->data;

                // try to allocate memory
                pcb->memory = (block_t *)mm_malloc(memory, pcb->memory_size);
                if (pcb->memory) {
                    // if memory was successfully allocated, move the
                    // process to the ready queue
                    if (DEBUG) {
                        printf("ACTION: Process %s successfully allocated "
                               "memory\n",
                               pcb->name);
                    }
                    printf("%" PRIu32
                           ",READY,process_name=%s,assigned_at=%" PRIu16 "\n",
                           simulation_time, pcb->name, pcb->memory->location);
                    move_data(pcb, input_queue, ready_queue);
                    pcb->state = READY;
                }
            }

            // free temporary queue
            free_list(temp_queue, NULL);
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
                    if (pcb->remaining_time < min_pcb->remaining_time) {
                        min = curr;
                    } else if (pcb->remaining_time == min_pcb->remaining_time) {
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
                    printf("%" PRIu32
                           ",RUNNING,process_name=%s,remaining_time=%" PRIu32
                           "\n",
                           simulation_time, pcb->name, pcb->remaining_time);
                    move_data(min->data, ready_queue, running_queue);
                    pcb->state = RUNNING;
                    // task4: start process
                    convert_to_big_endian(simulation_time,
                                          big_endian_simulation_time);
                    start_process(pcb->process, big_endian_simulation_time);
                }
            } else {
                // if there is a process currently running, continue to run
                // task4: continue process
                pcb_t *pcb = (pcb_t *)running_queue->head->data;
                convert_to_big_endian(simulation_time,
                                      big_endian_simulation_time);
                resume_process(pcb->process, big_endian_simulation_time);
            }
        } else if (strcmp(args->scheduler, "RR") == 0) {
            // if there are no processes in the ready queue, the process
            // that is currently running continues to run
            if (ready_queue->head == NULL) {
                // let the process continue to run
                // task4: continue process
                if (running_queue->head != NULL) {
                    pcb_t *pcb = (pcb_t *)running_queue->head->data;
                    convert_to_big_endian(simulation_time,
                                          big_endian_simulation_time);
                    resume_process(pcb->process, big_endian_simulation_time);
                }
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
                    pcb->state = SUSPENDED;
                    // task4: suspend process
                    convert_to_big_endian(simulation_time,
                                          big_endian_simulation_time);
                    suspend_process(pcb->process, big_endian_simulation_time);
                }

                // the process at the head of ready queue is chosen to run for
                // one quantum
                pcb_t *pcb = (pcb_t *)ready_queue->head->data;
                if (DEBUG) {
                    printf("ACTION: Adding process %s to running queue\n",
                           pcb->name);
                }
                printf("%" PRIu32
                       ",RUNNING,process_name=%s,remaining_time=%" PRIu32 "\n",
                       simulation_time, pcb->name, pcb->remaining_time);
                move_data(pcb, ready_queue, running_queue);
                // task4: start process or resume process
                convert_to_big_endian(simulation_time,
                                      big_endian_simulation_time);
                if (pcb->state == READY) {
                    start_process(pcb->process, big_endian_simulation_time);
                } else if (pcb->state == SUSPENDED) {
                    resume_process(pcb->process, big_endian_simulation_time);
                } else {
                    fprintf(stderr,
                            "ERROR: Process %s is in an invalid state for "
                            "running\n",
                            pcb->name);
                    exit(EXIT_FAILURE);
                }
                pcb->state = RUNNING;
            }
        }

        // when the finished queue contains all processes, the process manager
        // can terminate
        if (list_len(finished_queue) == num_processes) {
            break;
        }

        simulation_time += quantum;
    }

    // performance statistics
    uint32_t turnaround = average_turnaround_time(finished_queue);
    float max_overhead = max_time_overhead(finished_queue);
    float overhead = average_time_overhead(finished_queue);
    printf("Turnaround time %" PRIu32
           "\nTime overhead %.2f %.2f\nMakespan %" PRIu32 "\n",
           turnaround, max_overhead, overhead, simulation_time);

    // free linked lists
    free_list(memory, free);
    free_list(submitted_queue, NULL);
    free_list(input_queue, NULL);
    free_list(ready_queue, NULL);
    free_list(running_queue, NULL);
    free_list(finished_queue, NULL);
    free(big_endian_simulation_time);
}

uint32_t average_turnaround_time(list_t *finished_queue) {
    /*  Average time (in seconds, rounded up to an integer) between the time
        when the process is completed and when it arrived.
     */
    uint32_t turnaround = 0;
    node_t *curr = finished_queue->head;
    while (curr != NULL) {
        pcb_t *pcb = (pcb_t *)curr->data;
        turnaround += pcb->termination_time - pcb->arrival_time;
        curr = curr->next;
    }
    int total_pcbs = list_len(finished_queue);
    assert(total_pcbs > 0);

    // read more at https://stackoverflow.com/a/2422722
    turnaround = (turnaround + total_pcbs - 1) / total_pcbs;

    return turnaround;
}

float max_time_overhead(list_t *finished_queue) {
    /*  Maximum time overhead when running a process, where overhead
        is defined as the turnaround time of the process divided by
        its service time.
     */
    float overhead;
    float max_overhead = 0;
    node_t *curr = finished_queue->head;
    while (curr != NULL) {
        pcb_t *pcb = (pcb_t *)curr->data;
        overhead = (pcb->termination_time - pcb->arrival_time) /
                   (float)pcb->service_time;
        if (overhead > max_overhead) {
            max_overhead = overhead;
        }
        curr = curr->next;
    }

    return max_overhead;
}

float average_time_overhead(list_t *finished_queue) {
    /*  Average time overhead when running a process, where overhead
        is defined as the turnaround time of the process divided by
        its service time.
     */
    float overhead = 0;
    float average_overhead = 0;
    node_t *curr = finished_queue->head;
    while (curr != NULL) {
        pcb_t *pcb = (pcb_t *)curr->data;
        overhead += (pcb->termination_time - pcb->arrival_time) /
                    (float)pcb->service_time;
        curr = curr->next;
    }
    int total_pcbs = list_len(finished_queue);
    assert(total_pcbs > 0);

    average_overhead = overhead / total_pcbs;

    return average_overhead;
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
    assert(args);
    args->file = read_flag("-f", NULL, argc, argv);
    args->scheduler = read_flag("-s", SCHEDULERS, argc, argv);
    args->memory = read_flag("-m", MEMORY_METHODS, argc, argv);
    args->quantum = read_flag("-q", QUANTUMS, argc, argv);
    return args;
}

void convert_to_big_endian(uint32_t value, char *big_endian) {
    /*  Convert a 32-bit integer to big endian and store it in a char array of
        length 4.
     */
    value = htonl(value);
    memcpy(big_endian, &value, sizeof(value));
}

/* =============================================================================
   Written by David Sha.
============================================================================= */
