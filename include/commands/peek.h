#ifndef PEEK_H_
#define PEEK_H_

#include <sys/stat.h> // For mode_t
#include <stdbool.h>

/**
 * @brief Prints file permissions in rwxrwxrwx format.
 * @param perms The mode_t value containing permission bits.
 */
void print_file_permissions(mode_t perms);

/**
 * @brief Executes the peek command with specified options.
 *
 * This function consolidates the logic for peek, peek -l, peek -a, peek -la.
 *
 * @param dir_arg The directory to peek into. If NULL or empty, peeks current directory.
 * @param home_dir The user's home directory (for ~ expansion).
 * @param list_long Format similar to 'ls -l'.
 * @param show_hidden Show hidden files (starting with '.').
 */
void peek_execute(const char* dir_arg, const char* home_dir, bool list_long, bool show_hidden);

#endif // PEEK_H_