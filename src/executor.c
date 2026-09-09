#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

#include "executor.h"
#include "builtins.h"

static volatile sig_atomic_t current_foreground_pid = 0;

void set_foreground_pid(pid_t pid) {
    current_foreground_pid = pid;
}

pid_t get_foreground_pid(void) {
    return current_foreground_pid;
}

/*
 * Checks for any background children that have completed execution
 * and reaps them with WNOHANG to prevent zombie processes.
 */
void reap_background_processes(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("[Process %d finished]\n", (int)pid);
    }
}

/*
 * Sets up I/O redirection for a command using open(), dup2(), and close().
 * Returns 0 on success, or -1 on error.
 */
static int apply_redirection(const Command *cmd) {
    /* Input Redirection (<) */
    if (cmd->input_file != NULL) {
        int fd_in = open(cmd->input_file, O_RDONLY);
        if (fd_in < 0) {
            fprintf(stderr, "ShellForge: %s: %s\n", cmd->input_file, strerror(errno));
            return -1;
        }
        if (dup2(fd_in, STDIN_FILENO) < 0) {
            perror("ShellForge: dup2 input error");
            close(fd_in);
            return -1;
        }
        close(fd_in);
    }

    /* Output Redirection (> or >>) */
    if (cmd->output_file != NULL) {
        int flags = O_WRONLY | O_CREAT;
        if (cmd->append_output) {
            flags |= O_APPEND; /* Append mode (>>) */
        } else {
            flags |= O_TRUNC;  /* Overwrite mode (>) */
        }

        int fd_out = open(cmd->output_file, flags, 0644);
        if (fd_out < 0) {
            fprintf(stderr, "ShellForge: %s: %s\n", cmd->output_file, strerror(errno));
            return -1;
        }
        if (dup2(fd_out, STDOUT_FILENO) < 0) {
            perror("ShellForge: dup2 output error");
            close(fd_out);
            return -1;
        }
        close(fd_out);
    }

    return 0;
}

/*
 * Executes a single command (num_commands == 1).
 */
static int execute_single_command(Pipeline *pipeline) {
    Command *cmd = &pipeline->commands[0];

    if (cmd->argc == 0 || cmd->args[0] == NULL) {
        return 0;
    }

    /*
     * If the command is a built-in and NOT in the background:
     * Execute it in the parent process (especially required for cd and exit).
     */
    if (!pipeline->is_background && is_builtin(cmd->args[0])) {
        /* Handle redirection for built-ins in the parent process if requested */
        int saved_stdin = -1;
        int saved_stdout = -1;
        int redirect_needed = (cmd->input_file != NULL || cmd->output_file != NULL);

        if (redirect_needed) {
            saved_stdin = dup(STDIN_FILENO);
            saved_stdout = dup(STDOUT_FILENO);
            if (apply_redirection(cmd) < 0) {
                if (saved_stdin >= 0) {
                    dup2(saved_stdin, STDIN_FILENO);
                    close(saved_stdin);
                }
                if (saved_stdout >= 0) {
                    dup2(saved_stdout, STDOUT_FILENO);
                    close(saved_stdout);
                }
                return 1;
            }
        }

        int ret = execute_builtin(cmd->args);

        if (redirect_needed) {
            if (saved_stdin >= 0) {
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            if (saved_stdout >= 0) {
                dup2(saved_stdout, STDOUT_FILENO);
                close(saved_stdout);
            }
        }

        return ret;
    }

    /*
     * For external commands (or built-ins executed in background):
     * Standard Unix fork-exec-wait lifecycle.
     */
    pid_t pid = fork();

    if (pid < 0) {
        perror("ShellForge: fork error");
        return -1;
    } else if (pid == 0) {
        /* CHILD PROCESS */
        /* Restore default signal handler so foreground child can be interrupted by Ctrl+C */
        signal(SIGINT, SIG_DFL);

        /* Apply any file redirection (<, >, >>) */
        if (apply_redirection(cmd) < 0) {
            exit(EXIT_FAILURE);
        }

        /* If it's a built-in running in a child (e.g. background built-in) */
        if (is_builtin(cmd->args[0])) {
            exit(execute_builtin(cmd->args));
        }

        /* Execute external program image */
        if (execvp(cmd->args[0], cmd->args) == -1) {
            if (errno == ENOENT) {
                fprintf(stderr, "ShellForge: command not found: %s\n", cmd->args[0]);
            } else {
                fprintf(stderr, "ShellForge: %s: %s\n", cmd->args[0], strerror(errno));
            }
            exit(127);
        }
    } else {
        /* PARENT PROCESS */
        if (pipeline->is_background) {
            /* Background command: do not block, print background process ID */
            printf("[Background PID: %d]\n", (int)pid);
            return 0;
        } else {
            /* Foreground command: block and wait for completion */
            set_foreground_pid(pid);
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                if (errno != EINTR) {
                    perror("ShellForge: waitpid error");
                }
            }
            set_foreground_pid(0);

            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            }
        }
    }

    return 0;
}

/*
 * Executes a pipeline of multiple commands connected by pipes (|).
 * Uses pipe(), fork(), dup2(), close(), and waitpid().
 */
static int execute_multi_pipeline(Pipeline *pipeline) {
    int num_cmds = pipeline->num_commands;
    int num_pipes = num_cmds - 1;
    int pipefds[2 * (MAX_COMMANDS - 1)];
    pid_t pids[MAX_COMMANDS];

    /* Create all necessary inter-process pipes */
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("ShellForge: pipe error");
            /* Close already opened pipes before returning */
            for (int j = 0; j < i * 2; j++) {
                close(pipefds[j]);
            }
            return -1;
        }
    }

    /* Spawn a child process for each command in the pipeline */
    for (int i = 0; i < num_cmds; i++) {
        Command *cmd = &pipeline->commands[i];
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("ShellForge: fork error in pipeline");
            /* Close all pipes */
            for (int j = 0; j < 2 * num_pipes; j++) {
                close(pipefds[j]);
            }
            return -1;
        }

        if (pids[i] == 0) {
            /* -------------------------------------------------------------
             * PIPELINE CHILD PROCESS
             * -------------------------------------------------------------
             */
            signal(SIGINT, SIG_DFL);

            /* If not first command: redirect stdin from previous pipe's read end */
            if (i > 0) {
                if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) < 0) {
                    perror("ShellForge: dup2 pipe read error");
                    exit(EXIT_FAILURE);
                }
            }

            /* If not last command: redirect stdout to current pipe's write end */
            if (i < num_cmds - 1) {
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0) {
                    perror("ShellForge: dup2 pipe write error");
                    exit(EXIT_FAILURE);
                }
            }

            /* Apply file redirection if specified for this command */
            if (apply_redirection(cmd) < 0) {
                exit(EXIT_FAILURE);
            }

            /* Close all pipe file descriptors in the child process */
            for (int j = 0; j < 2 * num_pipes; j++) {
                close(pipefds[j]);
            }

            /* Execute built-in or external command */
            if (is_builtin(cmd->args[0])) {
                exit(execute_builtin(cmd->args));
            }

            if (execvp(cmd->args[0], cmd->args) == -1) {
                if (errno == ENOENT) {
                    fprintf(stderr, "ShellForge: command not found: %s\n", cmd->args[0]);
                } else {
                    fprintf(stderr, "ShellForge: %s: %s\n", cmd->args[0], strerror(errno));
                }
                exit(127);
            }
        }
    }

    /* -------------------------------------------------------------
     * PIPELINE PARENT PROCESS
     * -------------------------------------------------------------
     * Close all pipe file descriptors in parent so EOF propagates properly.
     */
    for (int j = 0; j < 2 * num_pipes; j++) {
        close(pipefds[j]);
    }

    if (pipeline->is_background) {
        printf("[Background Pipeline PID: %d]\n", (int)pids[num_cmds - 1]);
        return 0;
    }

    /* Wait for all children in the pipeline */
    int last_exit_status = 0;
    for (int i = 0; i < num_cmds; i++) {
        set_foreground_pid(pids[i]);
        int status;
        if (waitpid(pids[i], &status, 0) == -1) {
            if (errno != EINTR) {
                perror("ShellForge: waitpid pipeline error");
            }
        }
        if (i == num_cmds - 1) {
            if (WIFEXITED(status)) {
                last_exit_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                last_exit_status = 128 + WTERMSIG(status);
            }
        }
    }
    set_foreground_pid(0);

    return last_exit_status;
}

/*
 * Public dispatcher for a parsed pipeline.
 */
int execute_pipeline(Pipeline *pipeline) {
    if (pipeline == NULL || pipeline->num_commands == 0) {
        return 0;
    }

    if (pipeline->num_commands == 1) {
        return execute_single_command(pipeline);
    }

    return execute_multi_pipeline(pipeline);
}

/*
 * Review 2 backwards compatibility functions.
 */
int execute_command(char **args) {
    if (args == NULL || args[0] == NULL) {
        return 0;
    }
    Pipeline pipeline;
    memset(&pipeline, 0, sizeof(Pipeline));
    pipeline.num_commands = 1;
    Command *cmd = &pipeline.commands[0];
    int count = 0;
    while (args[count] != NULL && count < MAX_ARGS - 1) {
        cmd->args[count] = args[count];
        count++;
    }
    cmd->args[count] = NULL;
    cmd->argc = count;
    return execute_pipeline(&pipeline);
}

int execute_external(char **args) {
    return execute_command(args);
}
