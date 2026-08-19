#include <stdio.h>
#include <string.h>
#include "parser.h"

/*
 * Checks if a token contains unsupported operators reserved for Review 3.
 * Returns 1 if unsupported feature found, 0 otherwise.
 */
static int is_unsupported_feature(const char *token) {
    if (strcmp(token, "|") == 0 ||
        strcmp(token, "<") == 0 ||
        strcmp(token, ">") == 0 ||
        strcmp(token, ">>") == 0 ||
        strcmp(token, "&") == 0 ||
        strcmp(token, ";") == 0 ||
        strcmp(token, "&&") == 0 ||
        strcmp(token, "||") == 0) {
        return 1;
    }

    /* Check for embedded operators or substitution prefixes */
    if (strchr(token, '|') != NULL ||
        strchr(token, '<') != NULL ||
        strchr(token, '>') != NULL ||
        strchr(token, '&') != NULL ||
        strchr(token, '`') != NULL ||
        strstr(token, "$(") != NULL) {
        return 1;
    }

    return 0;
}

/*
 * Tokenizes the input command string into an array of arguments (args).
 * Splits on whitespace (spaces, tabs, newlines).
 * Returns:
 *   0 on success
 *  -1 if unsupported syntax was encountered
 *   1 if the input was empty
 */
int parse_input(char *line, char **args, int max_args) {
    int count = 0;
    char *token = strtok(line, TOKEN_DELIMITERS);

    while (token != NULL) {
        /* Check if the user entered any Review 3 advanced operators */
        if (is_unsupported_feature(token)) {
            printf("ShellForge: Feature not implemented yet.\n");
            args[0] = NULL;
            return -1;
        }

        if (count >= max_args - 1) {
            fprintf(stderr, "ShellForge: error: too many arguments (max %d)\n", max_args - 1);
            break;
        }

        args[count++] = token;
        token = strtok(NULL, TOKEN_DELIMITERS);
    }

    args[count] = NULL;

    if (count == 0) {
        return 1; /* Empty input */
    }

    return 0; /* Success */
}
