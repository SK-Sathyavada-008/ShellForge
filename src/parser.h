#ifndef PARSER_H
#define PARSER_H

#define MAX_LINE_LEN 1024
#define MAX_ARGS 64
#define TOKEN_DELIMITERS " \t\r\n\a"

/*
 * Return status codes from parse_input:
 *  0: Successfully parsed tokens
 * -1: Unsupported feature detected (e.g., pipes, redirection, background execution)
 *  1: Empty line / no command
 */
int parse_input(char *line, char **args, int max_args);

#endif /* PARSER_H */
