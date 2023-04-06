#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <string.h>

void create_process(char *simulation_time);
void suspend_process(char *simulation_time);
void resume_process(char *simulation_time);
void terminate_process(char *simulation_time);

int main(void) {

    char simulation_time[4] = {0, 0, 0, 0};
    uint32_t value = 3215;
    value = htonl(value);
    memcpy(simulation_time, &value, sizeof(value));
    // printf("%02x %02x %02x %02x\n", (uint8_t)simulation_time[0],
    //        (uint8_t)simulation_time[1], (uint8_t)simulation_time[2],
    //        (uint8_t)simulation_time[3]);

    create_process(simulation_time);

    return 0;
}

char *cmd[] = {"./process", "-v", "TEST_PROCESS", NULL};

void create_process(char *simulation_time) {
    /*  1. fork() and exec an instance of process.
        2. Send 32 bit simulation time of when the process starts running
           to the standard input of the executable ./process, in Big Endian
           Byte Ordering, using pipe() and dup2().
        3. Read 1 byte from the standard output of the executable ./process
           and verify that it's the same as the least significant byte that
           was sent.
     */
    int parent_to_child_fd[2], child_to_parent_fd[2];
    pid_t pid;

    // create pipe
    if (pipe(parent_to_child_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(child_to_parent_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    switch (pid = fork()) {

    case 0: // child
        close(parent_to_child_fd[1]);
        close(child_to_parent_fd[0]);

        // read from parent
        dup2(parent_to_child_fd[0], STDIN_FILENO);

        // write to parent
        dup2(child_to_parent_fd[1], STDOUT_FILENO);

        // execute process
        execvp(cmd[0], cmd);
        perror("execvp");
        exit(EXIT_FAILURE);

    default: // parent
        close(parent_to_child_fd[0]);
        close(child_to_parent_fd[1]);

        // send the 32 bit simulation time to child process
        write(parent_to_child_fd[1], simulation_time, 4);

        // get the least significant byte from child process
        char output[1];
        read(child_to_parent_fd[0], output, 1);

        // check if the least significant byte is the same
        if (output[0] != simulation_time[3]) {
            fprintf(stderr,
                    "output is not the same as the least significant byte\n");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);

    case -1: // error
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void suspend_process(char *simulation_time) {
    /*  1. Send the 32 bit simulation time of when the process is suspended
           to the standard input of the executable ./process, in Big Endian
           Byte Ordering.
        2. Send a SIGSTOP signal to the process and wait for ./process
           to enter a stopped state.
     */
}

void resume_process(char *simulation_time) {
    /*  1. Send the 32 bit simulation time of when the process is resumed
           to the standard input of the executable ./process, in Big Endian
           Byte Ordering.
        2. Send a SIGCONT signal to the process.
        3. Read 1 byte from the standard output of the executable ./process
           and verify that it's the same as the least significant byte that
           was sent.
     */
}

void terminate_process(char *simulation_time) {
    /*  1. Send the 32 bit simulation time of when the process is terminated
           to the standard input of the executable ./process, in Big Endian
           Byte Ordering.
        2. Send a SIGTERM signal to the process.
        3. Read a 64 byte string from the standard output of the executable
           ./process and print it to the standard output.
     */
}