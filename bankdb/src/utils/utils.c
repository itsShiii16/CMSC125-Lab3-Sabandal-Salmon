#include "utils.h"

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

/*
 * Global verbose flag.
 *
 * Default:
 * 0 = disabled
 * 1 = enabled
 *
 * This can later be updated in main.c after parsing CLI arguments.
 */
int verbose_mode = 0;

/*
 * Prints an error message and exits the program immediately.
 *
 * Use this for unrecoverable errors such as failed allocations
 * or critical initialization failures.
 */
void error_exit(const char *message) {
    if (message != NULL) {
        fprintf(stderr, "Error: %s\n", message);
    } else {
        fprintf(stderr, "Error: unknown fatal error\n");
    }

    exit(EXIT_FAILURE);
}

/*
 * Allocates memory safely.
 *
 * If malloc fails, the program exits instead of continuing
 * with a NULL pointer.
 *
 * Returns:
 * - pointer to allocated memory on success
 */
void *safe_malloc(size_t size) {
    void *ptr = malloc(size);

    if (ptr == NULL) {
        error_exit("memory allocation failed");
    }

    return ptr;
}

/*
 * Removes trailing newline and trailing whitespace characters
 * from a string.
 *
 * Useful after reading lines using fgets().
 */
void trim_newline(char *str) {
    if (str == NULL) {
        return;
    }

    size_t len = strlen(str);

    while (len > 0) {
        char ch = str[len - 1];

        if (ch == '\n' || ch == '\r' || isspace((unsigned char) ch)) {
            str[len - 1] = '\0';
            len--;
        } else {
            break;
        }
    }
}

/*
 * Checks whether a string is empty or contains only whitespace.
 *
 * Returns:
 * - true if empty / whitespace only
 * - false otherwise
 */
bool is_empty_line(const char *str) {
    if (str == NULL) {
        return true;
    }

    while (*str != '\0') {
        if (!isspace((unsigned char) *str)) {
            return false;
        }
        str++;
    }

    return true;
}

/*
 * Prints a formatted debug message only when verbose mode is enabled.
 *
 * Example use:
 *   debug_log("Loaded account %d with balance %d\n", id, balance);
 */
void debug_log(const char *format, ...) {
    if (!verbose_mode || format == NULL) {
        return;
    }

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
