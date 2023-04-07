#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "process-api.h"

void free_process(void *data) {
    /*  Free a process_t struct.
     */
    process_t *process = (process_t *)data;
    free(process);
}

void print_process(void *data) {
    /*  Print a process_t struct.
     */
    process_t *process = (process_t *)data;
    printf("%d", process->pid);
}

void send_message(process_t *process, char *message) {
    /*  Send a message to a process.
     */
    if (write(process->to_process[1], message, 4) == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
}

void receive_message(process_t *process, char *message, int length) {
    /*  Receive a message from a process.
     */
    if (read(process->to_manager[0], message, length) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
}

void check_process(process_t *process, char *simulation_time) {
    /*  Check that a process is still running by sending a message
        and checking that the least significant bit of the message
        is the same as the output from the process executable.
     */
    char response[1] = {0};
    receive_message(process, response, 1);

    // check response is same as least significant bit of message
    if (response[0] != simulation_time[3]) {
        printf("Error: Big Endian ordering did not pass correctly.\n");
        exit(EXIT_FAILURE);
    }
}

void start_process(process_t *process, char *simulation_time) {
    /*  Send the simulation time as a message to a process
        to start the process.

        Check that the process was started correctly by checking
        that the least significant bit of the message is the same
        as the output from the process executable.
     */
    send_message(process, simulation_time);

    // check that the process was started correctly
    check_process(process, simulation_time);
}

void suspend_process(process_t *process, char *simulation_time) {
    /*  Send a simulation time as a message to a process. Then
        suspend the process by sending a SIGTSTP signal.
     */
    send_message(process, simulation_time);

    // suspend process
    int wstatus;
    kill(process->pid, SIGTSTP);
    pid_t w = waitpid(process->pid, &wstatus, WUNTRACED);
    if (w == -1) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }
    if (!WIFSTOPPED(wstatus)) {
        printf("Error: Process did not stop.\n");
        exit(EXIT_FAILURE);
    }
}

void resume_process(process_t *process, char *simulation_time) {
    /*  Send a simulation time as a message to a process. Then
        resume the process by sending a SIGCONT signal.

        Check that the process was resumed correctly by checking
        that the least significant bit of the message is the same
        as the output from the process executable.
     */
    send_message(process, simulation_time);

    // resume process
    kill(process->pid, SIGCONT);

    // check that the process was resumed correctly
    check_process(process, simulation_time);
}

char *terminate_process(process_t *process, char *simulation_time) {
    /*  Send a simulation time as a message to a process. Then
        terminate the process by sending a SIGTERM signal.

        Then read a 64 byte string from stdout of process executable
        and include in execution transcript.

        Return the string.
     */
    send_message(process, simulation_time);

    // terminate process
    kill(process->pid, SIGTERM);

    // read 64 byte string from stdout of process executable
    char *string = (char *)calloc(64 + 1, sizeof(char));
    assert(string);
    receive_message(process, string, 64);

    // free process
    free_process(process);

    return string;
}

/* =============================================================================
   Written by David Sha.
============================================================================= */
