#ifndef SHELL_HEADERS_H_
#define SHELL_HEADERS_H_

#include <stdio.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/types.h> // For mode_t, pid_t
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <ctype.h> // For isspace

// Project headers (found via -Iinclude in Makefile)
#include "prompt.h"
#include "warp.h"
#include "peek.h"
#include "que.h"    // Includes pastevents-like functionality
#include "proclore.h"
#include "seek.h"

// Color codes
#define _RED_     "\x1b[31m"
#define _GREEN_   "\x1b[32m"
#define _YELLOW_  "\x1b[33m"
#define _BLUE_    "\x1b[34m"
#define _MAGENTA_ "\x1b[35m"
#define _CYAN_    "\x1b[36m"
#define _RESET_   "\x1b[0m"

// Common buffer sizes and limits
#define MAX_PATH_LEN 4096
#define MAX_INPUT_LEN 4096
#define MAX_COMMAND_LEN 4096 // For individual commands after splitting by ;
#define MAX_ARGS 64          // Increased max args
#define MAX_BG_PROCS 100
#define HISTORY_SIZE 15      // Max items in history queue
#define HISTORY_FILENAME ".shellby_history.txt" // History file name

/**
 * @brief Prints a formatted error message to stderr.
 * @param message The error message string.
 */
void print_shell_error(const char* message);

/**
 * @brief Prints a formatted error message with perror style (includes system error) to stderr.
 * @param message The custom part of the error message.
 */
void print_shell_perror(const char* message);


#endif // SHELL_HEADERS_H_