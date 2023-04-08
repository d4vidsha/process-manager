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

    // run simulation via the process manager
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
        printf("ACTION: Reading in file...\n");
    }

    // read in file
    FILE *fp = fopen(args->file, "r");
    assert(fp);

    // for each line in the file, parse it and add it to a linked list
    list_t *submitted_pcbs = create_empty_list();
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    pcb_t *pcb;
    while ((read = getline(&line, &len, fp)) != -1) {
        pcb = create_pcb(line);
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
    int total_processes = list_len(process_table);
    cycle_t *c = create_cycle(args);

    // copy processes from process table to submitted queue
    copy_list(process_table, c->submitted_queue);

    // on each cycle
    while (list_len(c->finished_queue) < total_processes) {
        if (DEBUG) {
            printf("%" PRIu32 "\n", c->simulation_time);
            printf("   memory: ");
            print_list(c->memory, print_block);
            printf("submitted: ");
            print_list(c->submitted_queue, print_pcb);
            printf("    input: ");
            print_list(c->input_queue, print_pcb);
            printf("    ready: ");
            print_list(c->ready_queue, print_pcb);
            printf("  running: ");
            print_list(c->running_queue, print_pcb);
            printf(" finished: ");
            print_list(c->finished_queue, print_pcb);
        }
        run_cycle(c);

        // increment simulation time if not finished with all processes
        if (list_len(c->finished_queue) < total_processes) {
            c->simulation_time += c->quantum;
        }
    }
    print_performance_statistics(c);
    free_cycle(c);
}

void run_cycle(cycle_t *c) {
    /*  This function runs a single cycle of the simulation.
     */

    // if current running process has completed, terminate it and
    // deallocate its memory
    manage_termination(c);

    // identify all processes that have been submitted since the last cycle
    // occurred and add them to the input queue in the order they appear
    // in the process file
    manage_arrival(c);

    // move processes from the input queue to the ready queue upon
    // successful memory allocation
    if (strcmp(c->args->memory, "infinite") == 0) {
        infinite(c);
    } else if (strcmp(c->args->memory, "best-fit") == 0) {
        bestfit(c);
    }

    // determine the process that will run in this cycle
    if (strcmp(c->args->scheduler, "SJF") == 0) {
        sjf(c);
    } else if (strcmp(c->args->scheduler, "RR") == 0) {
        rr(c);
    }
}

void manage_termination(cycle_t *c) {
    /*  If current running process (if any) has completed, terminate it and
        deallocate its memory. Assumes that the running queue can only
        contain one process.
     */
    node_t *curr = c->running_queue->head;

    // if there are no more running processes, break
    if (curr == NULL) {
        return;
    }

    // assuming there are more running processes, check if they
    // should be terminated
    pcb_t *pcb = (pcb_t *)curr->data;

    // decrement remaining time by the quantum. if remaining time
    // becomes negative, set it to 0
    if (pcb->remaining_time < c->quantum) {
        pcb->remaining_time = 0;
    } else {
        pcb->remaining_time -= c->quantum;
    }

    // if remaining time is not 0, do not terminate
    if (pcb->remaining_time != 0) {
        return;
    }

    // terminate process
    if (DEBUG) {
        printf("ACTION: Terminating process %s\n", pcb->name);
    }

    // deallocate memory if using best-fit
    if (strcmp(c->args->memory, "best-fit") == 0) {
        mm_free(c->memory, pcb->memory);
        pcb->memory = NULL;
    }

    printf("%" PRIu32 ",FINISHED,process_name=%s,proc_remaining=%d\n",
           c->simulation_time, pcb->name,
           list_len(c->input_queue) + list_len(c->ready_queue));

    // update the process manager's data structures
    move_data(pcb, c->running_queue, c->finished_queue);
    pcb->state = TERMINATED;
    pcb->termination_time = c->simulation_time;

    // task4: terminate process
    big_endian(c->simulation_time, c->big_endian);
    char *sha256 = terminate_process(pcb->process, c->big_endian);
    printf("%" PRIu32 ",FINISHED-PROCESS,process_name=%s,sha=%s\n",
           c->simulation_time, pcb->name, sha256);
    free(sha256);
}

void manage_arrival(cycle_t *c) {
    /*  Identify all processes that have been submitted since the last cycle
        occurred and add them to the input queue in the order they appear
        in the process file. A process is considered to have been submitted
        to the system if its arrival time is less than or equal to the
        current simulation time.
     */
    while (TRUE) {
        node_t *curr = c->submitted_queue->head;

        // if there are no more submittable processes, break
        if (curr == NULL) {
            break;
        }

        // assuming there are more submittable processes, check if they
        // should be added to the input queue
        pcb_t *pcb = (pcb_t *)curr->data;
        if (pcb->arrival_time <= c->simulation_time) {
            if (DEBUG) {
                printf("ACTION: Adding process %s to input queue\n", pcb->name);
            }
            move_data(pcb, c->submitted_queue, c->input_queue);
            pcb->state = NEW;

            // task4: initialise process
            initialise_process(pcb);
        } else {
            // assume that the processes are sorted by arrival time,
            // so if the current process has not arrived, none of the
            // remaining processes will be arrived, so we break
            break;
        }
    }
}

void infinite(cycle_t *c) {
    /*  Move all processes in the input queue to the ready queue assuming
        that there is infinite memory.
     */
    while (TRUE) {
        node_t *curr = c->input_queue->head;

        // if there are no more input processes, break
        if (curr == NULL) {
            break;
        }

        // assuming there are more input processes, move them
        // directly to the ready queue
        pcb_t *pcb = (pcb_t *)curr->data;
        if (DEBUG) {
            printf("ACTION: Adding process %s to ready queue\n", pcb->name);
        }
        move_data(pcb, c->input_queue, c->ready_queue);
        pcb->state = READY;
    }
}

void bestfit(cycle_t *c) {
    /*  Choose the best fit memory block for each process in the input queue
        and move the process to the ready queue upon successful memory
        allocation.
     */

    // copy input queue to a temporary queue so that for loops
    // can be used to iterate through the input queue
    list_t *temp_queue = create_empty_list();
    copy_list(c->input_queue, temp_queue);

    // try to allocate memory for each process in the input queue
    for (node_t *curr = temp_queue->head; curr != NULL; curr = curr->next) {
        pcb_t *pcb = (pcb_t *)curr->data;

        // try to allocate memory
        pcb->memory = (block_t *)mm_malloc(c->memory, pcb->memory_size);
        if (!pcb->memory) {
            continue;
        }

        // if memory was successfully allocated, move the process to the ready
        // queue
        if (DEBUG) {
            printf("ACTION: Process %s successfully allocated "
                   "memory\n",
                   pcb->name);
        }
        printf("%" PRIu32 ",READY,process_name=%s,assigned_at=%" PRIu16 "\n",
               c->simulation_time, pcb->name, pcb->memory->location);
        move_data(pcb, c->input_queue, c->ready_queue);
        pcb->state = READY;
    }

    // free temporary queue
    free_list(temp_queue, NULL);
}

void sjf(cycle_t *c) {
    /*  Shortest Job First (SJF) scheduling algorithm.

        Find the process with the shortest service time if two processes
        have the same service time, choose the process that arrived first.

        If two processes have the same service time and arrival time,
        choose the process whose name comes first lexicographically
     */
    if (c->running_queue->head == NULL) {

        // only add process to running queue if there is no running process
        node_t *curr = c->ready_queue->head;
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

        // check if there is a process to add to the running queue
        if (!min) {
            return;
        }

        // add process to running queue
        pcb_t *pcb = (pcb_t *)min->data;
        if (DEBUG) {
            printf("ACTION: Adding process %s to running queue\n", pcb->name);
        }
        printf("%" PRIu32 ",RUNNING,process_name=%s,remaining_time=%" PRIu32
               "\n",
               c->simulation_time, pcb->name, pcb->remaining_time);
        move_data(min->data, c->ready_queue, c->running_queue);
        pcb->state = RUNNING;

        // task4: start process
        big_endian(c->simulation_time, c->big_endian);
        start_process(pcb->process, c->big_endian);

    } else {
        // if there is a process currently running, continue to run
        // task4: continue process
        pcb_t *pcb = (pcb_t *)c->running_queue->head->data;
        big_endian(c->simulation_time, c->big_endian);
        continue_process(pcb->process, c->big_endian);
    }
}

void rr(cycle_t *c) {
    /*  Round Robin (RR) scheduling algorithm.

        If there is a process currently running, suspend and add it
        to the end of the ready queue, assuming only one running
        process exists at a time. Then choose the process at the
        front of the ready queue and add it to the running queue.

        If there are no processes in the ready queue, the process
        that is currently running continues to run.
     */
    if (c->ready_queue->head == NULL) {
        // let the process continue to run
        // task4: continue process
        if (c->running_queue->head != NULL) {
            pcb_t *pcb = (pcb_t *)c->running_queue->head->data;
            big_endian(c->simulation_time, c->big_endian);
            continue_process(pcb->process, c->big_endian);
        }
    } else {
        // if there is a process currently running, suspend and add it
        // to the end of the ready queue, assuming only one running
        // process exists at a time
        if (c->running_queue->head != NULL) {
            pcb_t *pcb = (pcb_t *)c->running_queue->head->data;
            if (DEBUG) {
                printf("ACTION: Suspending process %s\n", pcb->name);
            }
            move_data(pcb, c->running_queue, c->ready_queue);
            pcb->state = SUSPENDED;

            // task4: suspend process
            big_endian(c->simulation_time, c->big_endian);
            suspend_process(pcb->process, c->big_endian);
        }

        // the process at the head of ready queue is chosen to run for
        // one quantum
        pcb_t *pcb = (pcb_t *)c->ready_queue->head->data;
        if (DEBUG) {
            printf("ACTION: Adding process %s to running queue\n", pcb->name);
        }
        printf("%" PRIu32 ",RUNNING,process_name=%s,remaining_time=%" PRIu32
               "\n",
               c->simulation_time, pcb->name, pcb->remaining_time);
        move_data(pcb, c->ready_queue, c->running_queue);

        // task4: start process or resume process
        big_endian(c->simulation_time, c->big_endian);
        if (pcb->state == READY) {
            start_process(pcb->process, c->big_endian);
        } else if (pcb->state == SUSPENDED) {
            continue_process(pcb->process, c->big_endian);
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

cycle_t *create_cycle(args_t *args) {
    /*  Create a cycle struct.
     */
    cycle_t *c;
    c = (cycle_t *)malloc(sizeof(*c));
    assert(c);

    // initialise the cycle
    c->quantum = (uint32_t)atoi(args->quantum);
    c->simulation_time = 0;
    c->big_endian = calloc(BIG_ENDIAN_BYTES, sizeof(char));
    assert(c->big_endian);
    c->args = args;
    c->memory = mm_init(MAX_MEMORY);
    c->submitted_queue = create_empty_list();
    c->input_queue = create_empty_list();
    c->ready_queue = create_empty_list();
    c->running_queue = create_empty_list();
    c->finished_queue = create_empty_list();

    return c;
}

void free_cycle(cycle_t *c) {
    /*  Free a cycle struct.
     */
    free(c->big_endian);
    // assume the memory manager has no more memory allocated
    free_list(c->memory, free);
    free_list(c->submitted_queue, NULL);
    free_list(c->input_queue, NULL);
    free_list(c->ready_queue, NULL);
    free_list(c->running_queue, NULL);
    free_list(c->finished_queue, NULL);
    free(c);
}

void print_performance_statistics(cycle_t *c) {
    /*  Print the turnaround time, time overhead and makespan.
     */
    printf("Turnaround time %" PRIu32
           "\nTime overhead %.2f %.2f\nMakespan %" PRIu32 "\n",
           average_turnaround_time(c->finished_queue),
           max_time_overhead(c->finished_queue),
           average_time_overhead(c->finished_queue), c->simulation_time);
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
        assert(pcb->service_time > 0);
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
        assert(pcb->service_time > 0);
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

void big_endian(uint32_t value, char *big_endian) {
    /*  Convert a 32-bit integer to big endian and store it in a
        char array of length 4.
     */
    value = htonl(value);
    memcpy(big_endian, &value, sizeof(value));
}

/* =============================================================================
   Written by David Sha.
============================================================================= */
