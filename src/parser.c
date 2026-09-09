#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

#define MAX_TOKENS 256

/*
 * Checks if a token is one of the shell control operators.
 */
static int is_operator_token(const char *tok) {
    if (tok == NULL) {
        return 0;
    }
    return (strcmp(tok, "|") == 0 ||
            strcmp(tok, "<") == 0 ||
            strcmp(tok, ">") == 0 ||
            strcmp(tok, ">>") == 0 ||
            strcmp(tok, "&") == 0);
}

/*
 * Tokenizes the input line into an array of token strings.
 * Handles operators (| < > >> &) even if attached to words (e.g. ls|grep or ls>out).
 * Respects single and double quotes by keeping their contents as a single word.
 * Returns the total number of tokens found.
 */
static int tokenize_line(const char *line, char *tokens[], int max_tokens,
                         char *token_buf, size_t buf_size) {
    int count = 0;
    size_t buf_idx = 0;
    const char *p = line;

    while (*p != '\0') {
        /* Skip whitespace */
        while (*p != '\0' && isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '\0') {
            break;
        }

        if (count >= max_tokens - 1 || buf_idx + 4 >= buf_size) {
            break;
        }

        /* Check for >> */
        if (p[0] == '>' && p[1] == '>') {
            tokens[count++] = &token_buf[buf_idx];
            token_buf[buf_idx++] = '>';
            token_buf[buf_idx++] = '>';
            token_buf[buf_idx++] = '\0';
            p += 2;
            continue;
        }

        /* Check for single-character operators: >, <, |, & */
        if (*p == '>' || *p == '<' || *p == '|' || *p == '&') {
            tokens[count++] = &token_buf[buf_idx];
            token_buf[buf_idx++] = *p;
            token_buf[buf_idx++] = '\0';
            p++;
            continue;
        }

        /* Regular argument / word */
        tokens[count++] = &token_buf[buf_idx];
        while (*p != '\0' && !isspace((unsigned char)*p) &&
               *p != '|' && *p != '<' && *p != '>' && *p != '&') {
            if (*p == '"' || *p == '\'') {
                char quote = *p++;
                while (*p != '\0' && *p != quote) {
                    if (buf_idx + 1 < buf_size) {
                        token_buf[buf_idx++] = *p;
                    }
                    p++;
                }
                if (*p == quote) {
                    p++; /* Skip closing quote */
                }
            } else {
                if (buf_idx + 1 < buf_size) {
                    token_buf[buf_idx++] = *p;
                }
                p++;
            }
        }
        if (buf_idx < buf_size) {
            token_buf[buf_idx++] = '\0';
        }
    }

    tokens[count] = NULL;
    return count;
}

/*
 * Parses a command line string into a structured Pipeline.
 */
int parse_pipeline(char *line, Pipeline *pipeline) {
    static char token_buf[MAX_LINE_LEN * 2];
    char *tokens[MAX_TOKENS];

    memset(pipeline, 0, sizeof(Pipeline));

    int token_count = tokenize_line(line, tokens, MAX_TOKENS, token_buf, sizeof(token_buf));
    if (token_count == 0) {
        return 1; /* Empty command */
    }

    /* Check for trailing background execution '&' */
    if (strcmp(tokens[token_count - 1], "&") == 0) {
        pipeline->is_background = 1;
        token_count--;
        tokens[token_count] = NULL;

        if (token_count == 0) {
            fprintf(stderr, "ShellForge: syntax error near unexpected token '&'\n");
            return -1;
        }
    }

    /* Check for misplaced '&' anywhere else in the token stream */
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "&") == 0) {
            fprintf(stderr, "ShellForge: syntax error near unexpected token '&'\n");
            return -1;
        }
    }

    int cmd_idx = 0;
    int expect_command = 1;
    Command *current_cmd = &pipeline->commands[0];

    for (int i = 0; i < token_count; i++) {
        const char *tok = tokens[i];

        if (strcmp(tok, "|") == 0) {
            if (expect_command || current_cmd->argc == 0) {
                fprintf(stderr, "ShellForge: syntax error near unexpected token '|'\n");
                return -1;
            }

            current_cmd->args[current_cmd->argc] = NULL;
            cmd_idx++;

            if (cmd_idx >= MAX_COMMANDS) {
                fprintf(stderr, "ShellForge: error: too many commands in pipeline (max %d)\n", MAX_COMMANDS);
                return -1;
            }

            current_cmd = &pipeline->commands[cmd_idx];
            expect_command = 1;
            continue;
        }

        if (strcmp(tok, "<") == 0) {
            i++;
            if (i >= token_count || is_operator_token(tokens[i])) {
                fprintf(stderr, "ShellForge: syntax error near unexpected token '%s'\n",
                        (i < token_count) ? tokens[i] : "newline");
                return -1;
            }
            current_cmd->input_file = tokens[i];
            continue;
        }

        if (strcmp(tok, ">") == 0) {
            i++;
            if (i >= token_count || is_operator_token(tokens[i])) {
                fprintf(stderr, "ShellForge: syntax error near unexpected token '%s'\n",
                        (i < token_count) ? tokens[i] : "newline");
                return -1;
            }
            current_cmd->output_file = tokens[i];
            current_cmd->append_output = 0;
            continue;
        }

        if (strcmp(tok, ">>") == 0) {
            i++;
            if (i >= token_count || is_operator_token(tokens[i])) {
                fprintf(stderr, "ShellForge: syntax error near unexpected token '%s'\n",
                        (i < token_count) ? tokens[i] : "newline");
                return -1;
            }
            current_cmd->output_file = tokens[i];
            current_cmd->append_output = 1;
            continue;
        }

        /* Regular argument */
        if (current_cmd->argc >= MAX_ARGS - 1) {
            fprintf(stderr, "ShellForge: error: too many arguments (max %d)\n", MAX_ARGS - 1);
            return -1;
        }

        current_cmd->args[current_cmd->argc++] = tokens[i];
        expect_command = 0;
    }

    if (expect_command || current_cmd->argc == 0) {
        fprintf(stderr, "ShellForge: syntax error near unexpected token '|'\n");
        return -1;
    }

    current_cmd->args[current_cmd->argc] = NULL;
    pipeline->num_commands = cmd_idx + 1;

    return 0;
}

/*
 * Retained for backwards compatibility with Review 2 callers.
 */
int parse_input(char *line, char **args, int max_args) {
    int count = 0;
    char *token = strtok(line, TOKEN_DELIMITERS);

    while (token != NULL) {
        if (count >= max_args - 1) {
            break;
        }
        args[count++] = token;
        token = strtok(NULL, TOKEN_DELIMITERS);
    }

    args[count] = NULL;
    return (count == 0) ? 1 : 0;
}
