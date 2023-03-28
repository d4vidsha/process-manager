#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

const char *const SCHEDULERS[] = {"SJF", "RR", NULL};
const char *const MEMORY_METHODS[] = {"infinite", "best-fit", NULL};
const char *const QUANTUMS[] = {"1", "2", "3", NULL};

typedef struct args {
    char *file;
    char *scheduler;
    char *memory;
    char *quantum;
} args_t;

char *read_flag(char *flag, const char *const *valid_args, int argc,
                char *argv[]);
args_t parse_args(int argc, char *argv[]);
void simulate(args_t args);

int main(int argc, char *argv[]) {

    // read in flags and arguments
    args_t args = parse_args(argc, argv);
    assert(args.file != NULL);
    assert(args.scheduler != NULL);
    assert(args.memory != NULL);
    assert(args.quantum != NULL);

    // run simulation
    simulate(args);

    return 0;
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

void simulate(args_t args) {
    /*  Given a struct of arguments that contains the file, scheduler,
        memory method, and quantum, run the simulation.
    */
    printf("Running simulation with the following arguments:\n");
    printf("File: %s\n", args.file);
    printf("Scheduler: %s\n", args.scheduler);
    printf("Memory: %s\n", args.memory);
    printf("Quantum: %s\n", args.quantum);

    // read in file
    FILE *fp = fopen(args.file, "r");
    assert(fp);

    

}