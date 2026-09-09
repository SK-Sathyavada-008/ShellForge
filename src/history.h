#ifndef HISTORY_H
#define HISTORY_H

#define MAX_HISTORY 100

/*
 * Initializes the history data structures.
 */
void history_init(void);

/*
 * Adds a command string to the in-memory history.
 * Empty or whitespace-only commands are ignored.
 */
void history_add(const char *cmd);

/*
 * Displays all recorded commands with sequential 1-based index numbers.
 * Example:
 *   1  pwd
 *   2  ls -l
 *   3  history
 */
void history_print(void);

/*
 * Cleans up and frees all allocated history memory.
 */
void history_cleanup(void);

#endif /* HISTORY_H */
