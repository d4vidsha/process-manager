#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

int main(int argc, char *argv[]) {

    // read in flags and arguments
    args_t args = parse_args(argc, argv);
    assert(args.file != NULL);
    assert(args.scheduler != NULL);
    assert(args.memory != NULL);
    assert(args.quantum != NULL);

    // run simulation
    simulation(args);

    return 0;
}

void simulation(args_t args) {
    /*  Given a struct of arguments that contains the file, scheduler,
        memory method, and quantum, run the simulation.
    */
    if (DEBUG) {
        printf("ACTION: Running simulation with the following arguments...\n");
        printf("File: %s\n", args.file);
        printf("Scheduler: %s\n", args.scheduler);
        printf("Memory: %s\n", args.memory);
        printf("Quantum: %s\n", args.quantum);
    }

    // read in file
    if (DEBUG) {
        printf("ACTION: Reading in file...\n");
    }
    FILE *fp = fopen(args.file, "r");
    assert(fp);

    // for each line in the file, parse it and add it to the priority queue
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
        pcb_t pcb = parse_pcb_line(line);

        if (DEBUG) {
            printf("name: %s, arrive: %" PRIu32 "s, service: %" PRIu32
                   "s, mem: %" PRIu16 "MB\n",
                   pcb.name, pcb.arrival_time, pcb.service_time,
                   pcb.memory_size);
        }
    }

    int simulation_time = 0;
    int cycles = 0;

    // free line if necessary
    if (line) {
        free(line);
    }

    // close file
    fclose(fp);
}

void cycle_tasks() {
    /*  Run one cycle.
     */
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

args_t parse_args(int argc, char *argv[]) {
    /*  Given a list of arguments, parse them and return all flags and
        arguments in a struct.
    */
    args_t args;
    args.file = read_flag("-f", NULL, argc, argv);
    args.scheduler = read_flag("-s", SCHEDULERS, argc, argv);
    args.memory = read_flag("-m", MEMORY_METHODS, argc, argv);
    args.quantum = read_flag("-q", QUANTUMS, argc, argv);
    return args;
}

pcb_t parse_pcb_line(char *line) {
    /*  Given a line from the input file, parse it and return a pcb_t
        struct.

        The line should be in the format:
        <arrival time> <name> <service time> <memory size>
    */
    pcb_t pcb;
    char *token = strtok(line, SEPARATOR);
    pcb.arrival_time = (uint32_t)strtoul(token, NULL, 10);
    token = strtok(NULL, SEPARATOR);
    pcb.name = token;
    token = strtok(NULL, SEPARATOR);
    pcb.service_time = (uint32_t)strtoul(token, NULL, 10);
    token = strtok(NULL, SEPARATOR);
    pcb.memory_size = (uint16_t)strtoul(token, NULL, 10);
    return pcb;
}

/* =============================================================================
   Written by David Sha.
============================================================================= */
