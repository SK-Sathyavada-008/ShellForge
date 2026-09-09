#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "builtins.h"
#include "history.h"

/*
 * Checks if the command is one of the built-ins supported by ShellForge.
 */
int is_builtin(const char *cmd) {
    if (cmd == NULL) {
        return 0;
    }
    if (strcmp(cmd, "cd") == 0 ||
        strcmp(cmd, "pwd") == 0 ||
        strcmp(cmd, "exit") == 0 ||
        strcmp(cmd, "history") == 0) {
        return 1;
    }
    return 0;
}

/*
 * Built-in 'cd': changes the current working directory of the shell.
 * Must be executed in the parent process using the chdir() system call.
 */
int builtin_cd(char **args) {
    const char *target = args[1];

    /* If no path argument provided, default to HOME environment variable */
    if (target == NULL) {
        target = getenv("HOME");
        if (target == NULL) {
            fprintf(stderr, "ShellForge: cd: HOME not set\n");
            return 1;
        }
    }

    if (chdir(target) != 0) {
        fprintf(stderr, "ShellForge: cd: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

/*
 * Built-in 'pwd': prints the current working directory.
 * Uses the getcwd() POSIX function.
 */
int builtin_pwd(char **args) {
    (void)args; /* Unused parameter */
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("ShellForge: pwd");
        return 1;
    }
}

/*
 * Built-in 'exit': terminates the ShellForge process gracefully.
 */
int builtin_exit(char **args) {
    (void)args; /* Unused parameter */
    history_cleanup();
    printf("Goodbye!\n");
    exit(EXIT_SUCCESS);
    return 0;
}

/*
 * Built-in 'history': displays previous command history.
 */
int builtin_history(char **args) {
    (void)args;
    history_print();
    return 0;
}

/*
 * Routes and executes the matching built-in command.
 */
int execute_builtin(char **args) {
    if (args == NULL || args[0] == NULL) {
        return 0;
    }

    if (strcmp(args[0], "cd") == 0) {
        return builtin_cd(args);
    } else if (strcmp(args[0], "pwd") == 0) {
        return builtin_pwd(args);
    } else if (strcmp(args[0], "exit") == 0) {
        return builtin_exit(args);
    } else if (strcmp(args[0], "history") == 0) {
        return builtin_history(args);
    }

    return 0;
}
