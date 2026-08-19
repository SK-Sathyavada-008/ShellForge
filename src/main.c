#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "executor.h"

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
    char *args[MAX_ARGS];

    print_banner();

    while (1) {
        /* Display prompt */
        printf("ShellForge> ");
        fflush(stdout);

        /* Read user input */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            /* Handled EOF (e.g., Ctrl+D) */
            printf("\nGoodbye!\n");
            break;
        }

        /* Parse command line into arguments */
        int parse_status = parse_input(line, args, MAX_ARGS);

        /* If parsing succeeded (parse_status == 0), execute the command */
        if (parse_status == 0) {
            execute_command(args);
        }
        /* If parse_status == 1 (empty line) or -1 (unsupported feature), prompt repeats */
    }

    return 0;
}
