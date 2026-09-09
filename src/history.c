#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "history.h"

static char *history_entries[MAX_HISTORY];
static int history_count = 0;

void history_init(void) {
    for (int i = 0; i < MAX_HISTORY; i++) {
        history_entries[i] = NULL;
    }
    history_count = 0;
}

/*
 * Helper to check if a string is empty or contains only whitespace.
 */
static int is_blank_string(const char *str) {
    if (str == NULL) {
        return 1;
    }
    while (*str) {
        if (!isspace((unsigned char)*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}

void history_add(const char *cmd) {
    if (cmd == NULL || is_blank_string(cmd)) {
        return;
    }

    /* Trim leading whitespace */
    while (isspace((unsigned char)*cmd)) {
        cmd++;
    }

    /* Find length without trailing newline or trailing whitespace */
    size_t len = strlen(cmd);
    while (len > 0 && (cmd[len - 1] == '\n' || cmd[len - 1] == '\r' || isspace((unsigned char)cmd[len - 1]))) {
        len--;
    }

    if (len == 0) {
        return;
    }

    char *entry = (char *)malloc(len + 1);
    if (entry == NULL) {
        perror("ShellForge: history allocation failed");
        return;
    }
    strncpy(entry, cmd, len);
    entry[len] = '\0';

    if (history_count < MAX_HISTORY) {
        history_entries[history_count++] = entry;
    } else {
        /* Shift entries left to discard oldest entry */
        free(history_entries[0]);
        for (int i = 1; i < MAX_HISTORY; i++) {
            history_entries[i - 1] = history_entries[i];
        }
        history_entries[MAX_HISTORY - 1] = entry;
    }
}

void history_print(void) {
    for (int i = 0; i < history_count; i++) {
        if (history_entries[i] != NULL) {
            printf("%4d  %s\n", i + 1, history_entries[i]);
        }
    }
}

void history_cleanup(void) {
    for (int i = 0; i < MAX_HISTORY; i++) {
        if (history_entries[i] != NULL) {
            free(history_entries[i]);
            history_entries[i] = NULL;
        }
    }
    history_count = 0;
}
