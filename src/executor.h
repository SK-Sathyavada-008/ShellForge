#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <sys/types.h>
#include "parser.h"

/*
 * Executes a parsed pipeline containing 1 or more commands,
 * with optional input/output redirections and background execution.
 * Returns the exit status of the executed command/pipeline.
 */
int execute_pipeline(Pipeline *pipeline);

/*
 * Checks for any finished background processes and reaps them
 * using non-blocking waitpid(..., WNOHANG) to avoid zombies.
 */
void reap_background_processes(void);

/*
 * Getter and setter for the current foreground process ID,
 * used by the SIGINT signal handler to forward interrupt signals.
 */
void set_foreground_pid(pid_t pid);
pid_t get_foreground_pid(void);

/*
 * Review 2 backwards compatibility functions.
 */
int execute_command(char **args);
int execute_external(char **args);

#endif /* EXECUTOR_H */
