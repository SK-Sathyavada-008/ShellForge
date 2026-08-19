#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "executor.h"
#include "builtins.h"

/*
 * Executes an external program by creating a new child process.
 * Standard Unix Process Model:
 *   1. fork() creates an exact duplicate child process.
 *   2. Child calls execvp(), replacing its address space with the executable image.
 *   3. Parent calls waitpid() to block until the child finishes execution.
 */
int execute_external(char **args) {
    pid_t pid = fork();

    if (pid < 0) {
        /* Fork failed */
        perror("ShellForge: fork error");
        return -1;
    } else if (pid == 0) {
        /* -------------------------------------------------------------
         * CHILD PROCESS
         * -------------------------------------------------------------
         * execvp looks up the binary in PATH (or uses direct path)
         * and replaces the child process memory image.
         */
        if (execvp(args[0], args) == -1) {
            if (errno == ENOENT) {
                fprintf(stderr, "ShellForge: command not found: %s\n", args[0]);
            } else {
                fprintf(stderr, "ShellForge: %s: %s\n", args[0], strerror(errno));
            }
            /* Child must terminate immediately on exec failure */
            exit(127);
        }
    } else {
        /* -------------------------------------------------------------
         * PARENT PROCESS
         * -------------------------------------------------------------
         * The parent must wait for the child process to complete
         * to prevent zombie processes and ensure sequential command execution.
         */
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("ShellForge: waitpid error");
            return -1;
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);
        }
    }

    return 0;
}

/*
 * Decides whether to run a command as an internal built-in or external program.
 */
int execute_command(char **args) {
    if (args == NULL || args[0] == NULL) {
        return 0; /* Empty command, nothing to execute */
    }

    /* Check if command is a built-in handled directly by ShellForge */
    if (is_builtin(args[0])) {
        return execute_builtin(args);
    }

    /* Otherwise, dispatch to external process runner */
    return execute_external(args);
}
