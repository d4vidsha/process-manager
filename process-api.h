/* =============================================================================
   process-api.h

   The process struct is used to represent a child process when this program
   is executed.

   Author: David Sha
============================================================================= */
#ifndef _PROCESS_API_H_
#define _PROCESS_API_H_

/* #includes ================================================================ */
#include <sys/types.h>

/* structures =============================================================== */
typedef struct process {
    pid_t pid;
    int fd[2];
    int parent_fd[2];
} process_t;

/* function prototypes ====================================================== */
void send_message(process_t *process, char *message, int length);
void receive_message(process_t *process, char *message, int length);
void check_process(process_t *process, char *simulation_time);
void start_process(process_t *process, char *simulation_time);
void suspend_process(process_t *process, char *simulation_time);
void continue_process(process_t *process, char *simulation_time);
char *terminate_process(process_t *process, char *simulation_time);

#endif
