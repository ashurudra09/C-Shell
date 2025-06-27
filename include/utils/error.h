#ifndef ERROR_H_
#define ERROR_H_

#include "utils/colors.h" // <-- ADD THIS
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * REMOVE the old color code definitions from here.
 * They are now in utils/colors.h
 */

/**
 * @brief Prints a formatted error message to stderr.
 * @param message The error message string.
 */
void print_shell_error(const char* message);

/**
 * @brief Prints a formatted error message with perror style to stderr.
 * @param message The custom part of the error message.
 */
void print_shell_perror(const char* message);

#endif // ERROR_H_