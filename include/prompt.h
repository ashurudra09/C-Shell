#ifndef PROMPT_H_
#define PROMPT_H_

#include <sys/types.h> // For time_t if not implicitly included

/**
 * @brief Displays the shell prompt.
 * @param home_dir The home directory path of the shell user.
 * @param last_command_name The name of the last executed command (for timed prompt).
 * @param time_taken Time taken by the last command in seconds.
 */
void display_shell_prompt(const char* home_dir, const char* last_command_name, long time_taken);

#endif // PROMPT_H_