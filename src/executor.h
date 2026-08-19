#ifndef EXECUTOR_H
#define EXECUTOR_H

/*
 * Routes and executes a command (either built-in or external).
 * Returns 0 on success, or non-zero on error.
 */
int execute_command(char **args);

/*
 * Executes an external command using fork(), execvp(), and waitpid().
 * Returns child exit status or -1 on system call error.
 */
int execute_external(char **args);

#endif /* EXECUTOR_H */
