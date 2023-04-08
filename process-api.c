#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "config.h"
#include "process-api.h"

void send_message(process_t *process, char *message) {
    /*  Send a message to a process.
     */
    if (write(process->fd[1], message, BIG_ENDIAN_BYTES) == FAILED) {
        perror("write");
        exit(EXIT_FAILURE);
    }
}

void receive_message(process_t *process, char *message, int length) {
    /*  Receive a message from a process.
     */
    if (read(process->parent_fd[0], message, length) == FAILED) {
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
    if (response[0] != simulation_time[BIG_ENDIAN_BYTES - 1]) {
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
    if (kill(process->pid, SIGTSTP) == FAILED) {
        perror("kill");
        exit(EXIT_FAILURE);
    }
    do {
        if (waitpid(process->pid, &wstatus, WUNTRACED) == FAILED) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    } while (!WIFSTOPPED(wstatus));
}

void continue_process(process_t *process, char *simulation_time) {
    /*  Send a simulation time as a message to a process. Then
        resume/continue the process by sending a SIGCONT signal.

        Check that the process was continue correctly by checking
        that the least significant bit of the message is the same
        as the output from the process executable.
     */
    send_message(process, simulation_time);

    // continue process
    if (kill(process->pid, SIGCONT) == FAILED) {
        perror("kill");
        exit(EXIT_FAILURE);
    }

    // check that the process was continue correctly
    check_process(process, simulation_time);
}

char *terminate_process(process_t *process, char *simulation_time) {
    /*  Send a simulation time as a message to a process. Then
        terminate the process by sending a SIGTERM signal.

        Then read a 64 byte string from stdout of process executable
        and include in execution transcript.

        Return the string and free the process.
     */
    send_message(process, simulation_time);

    // terminate process
    if (kill(process->pid, SIGTERM) == FAILED) {
        perror("kill");
        exit(EXIT_FAILURE);
    }

    // read 64 byte string from stdout of process executable
    char *string = (char *)calloc(SHA256_LENGTH + 1, sizeof(char));
    assert(string);
    receive_message(process, string, SHA256_LENGTH);

    // close pipes
    if (close(process->fd[1]) == FAILED) {
        perror("close");
        exit(EXIT_FAILURE);
    }
    if (close(process->parent_fd[0]) == FAILED) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    // free process
    free(process);

    return string;
}

/* =============================================================================
   Written by David Sha.
============================================================================= */
