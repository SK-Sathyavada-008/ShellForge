#ifndef PARSER_H
#define PARSER_H

#define MAX_LINE_LEN 1024
#define MAX_ARGS 64
#define MAX_COMMANDS 16
#define TOKEN_DELIMITERS " \t\r\n\a"

/*
 * Represents a single command in a pipeline, including its arguments
 * and optional input/output file redirections.
 */
typedef struct {
    char *args[MAX_ARGS];  /* Null-terminated array of argument strings */
    int argc;              /* Number of arguments in args array */
    char *input_file;      /* Target file for '<' input redirection, or NULL */
    char *output_file;     /* Target file for '>' or '>>' redirection, or NULL */
    int append_output;     /* 1 if '>>' append mode, 0 if '>' truncate mode */
} Command;

/*
 * Represents an entire pipeline composed of one or more commands
 * separated by '|' pipes, with optional background execution flag '&'.
 */
typedef struct {
    Command commands[MAX_COMMANDS];
    int num_commands;
    int is_background;     /* 1 if command line ended with '&', 0 otherwise */
} Pipeline;

/*
 * Parses a raw command line into a structured Pipeline.
 *
 * Return status codes:
 *   0: Successfully parsed pipeline
 *   1: Empty line or whitespace-only
 *  -1: Syntax error encountered (printed to stderr)
 */
int parse_pipeline(char *line, Pipeline *pipeline);

/*
 * Retained for backwards compatibility with Review 2 callers.
 * Tokenizes simple whitespace-separated commands.
 */
int parse_input(char *line, char **args, int max_args);

#endif /* PARSER_H */
