#ifndef _PROCESS_API_H_
#define _PROCESS_API_H_

/* structures =============================================================== */
typedef struct process {
    pid_t pid;
    int to_process[2];
    int to_manager[2];
} process_t;

/* function prototypes ====================================================== */
void free_process(void *data);
void print_process(void *data);
void send_message(process_t *process, char *message);
void receive_message(process_t *process, char *message, int length);
void check_process(process_t *process, char *simulation_time);
void start_process(process_t *process, char *simulation_time);
void suspend_process(process_t *process, char *simulation_time);
void resume_process(process_t *process, char *simulation_time);
char *terminate_process(process_t *process, char *simulation_time);

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */
