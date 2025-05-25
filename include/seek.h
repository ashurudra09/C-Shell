#ifndef SEEK_H_
#define SEEK_H_

#include <stdbool.h>

/**
 * @brief Executes the seek command to find files/directories.
 *
 * @param target_name The name of the file or directory to seek.
 * @param search_dir_arg The directory to start searching from.
 * @param home_dir User's home directory (for ~ expansion and path relativization).
 * @param prev_dir Shell's previous directory (for 'warp' if -e on dir).
 * @param search_dirs_only True if only directories should be matched (-d flag).
 * @param search_files_only True if only files should be matched (-f flag).
 * @param execute_on_match True if an action should be performed on the first match (-e flag).
 *                         If a directory is found, changes to it.
 *                         If a file is found, prints its content.
 */
void seek_execute(const char* target_name, const char* search_dir_arg,
                  const char* home_dir, char* prev_dir,
                  bool search_dirs_only, bool search_files_only, bool execute_on_match);

#endif // SEEK_H_