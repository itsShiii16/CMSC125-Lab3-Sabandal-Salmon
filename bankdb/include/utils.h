#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * Utility Module
 *
 * Contains helper functions used across the system.
 */

/*
 * Prints an error message and exits the program.
 */
void error_exit(const char *message);

/*
 * Safe memory allocation wrapper.
 * Exits if allocation fails.
 */
void *safe_malloc(size_t size);

/*
 * Trims newline and trailing whitespace from a string.
 */
void trim_newline(char *str);

/*
 * Checks if a string is empty or only whitespace.
 */
bool is_empty_line(const char *str);

/*
 * Prints a formatted debug message if verbose mode is enabled.
 */
void debug_log(const char *format, ...);

/*
 * Global verbose flag (set via CLI).
 */
extern int verbose_mode;

#endif