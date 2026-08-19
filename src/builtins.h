#ifndef BUILTINS_H
#define BUILTINS_H

/*
 * Checks if the given command is a ShellForge built-in (cd, pwd, exit).
 * Returns 1 if built-in, 0 otherwise.
 */
int is_builtin(const char *cmd);

/*
 * Executes a built-in command directly within the parent process.
 * Returns 0 on success, or non-zero on failure.
 */
int execute_builtin(char **args);

/* Built-in function handlers */
int builtin_cd(char **args);
int builtin_pwd(char **args);
int builtin_exit(char **args);

#endif /* BUILTINS_H */
