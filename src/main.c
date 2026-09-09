#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "history.h"

/*
 * Signal handler for SIGINT (Ctrl+C).
 * - If a foreground child process is running, forward SIGINT to it.
 * - If the shell is idle at the prompt, print a newline and refresh prompt cleanly.
 */
static void sigint_handler(int sig) {
    (void)sig;
    pid_t fg = get_foreground_pid();

    if (fg > 0) {
        kill(fg, SIGINT);
    } else {
        const char msg[] = "\n";
        if (write(STDOUT_FILENO, msg, 1) < 0) {
            /* Ignore write error */
        }
    }
}

/*
 * Sets up POSIX signal handling for the shell.
 */
static void setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* Let system calls fail with EINTR so fgets can re-prompt */

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("ShellForge: sigaction SIGINT error");
    }
}

/*
 * Prints the welcome banner on shell startup.
 */
static void print_banner(void) {
    printf("================================\n");
    printf("          SHELLFORGE            \n");
    printf("================================\n\n");
}

/*
 * Main REPL (Read-Eval-Print Loop) for ShellForge.
 */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    char line[MAX_LINE_LEN];
    Pipeline pipeline;

    /* Initialize subsystems */
    history_init();
    setup_signals();
    print_banner();

    while (1) {
        /* Check and reap any completed background processes */
        reap_background_processes();

        /* Display prompt */
        printf("ShellForge> ");
        fflush(stdout);

        /* Read user input */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (errno == EINTR) {
                /* Prompt was interrupted by SIGINT (Ctrl+C) */
                clearerr(stdin);
                continue;
            }
            /* End of input stream (e.g., Ctrl+D / EOF) */
            printf("\nGoodbye!\n");
            break;
        }

        /* Parse command line into structured pipeline */
        int parse_status = parse_pipeline(line, &pipeline);

        /* Record valid command in history */
        if (parse_status == 0) {
            history_add(line);
            execute_pipeline(&pipeline);
        }
        /* If parse_status == 1 (empty line) or -1 (syntax error), loop repeats */
    }

    /* Cleanup resources */
    history_cleanup();
    return 0;
}
