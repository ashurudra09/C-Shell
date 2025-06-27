#ifndef IMAN_H_
#define IMAN_H_

/**
 * @brief Executes the 'iman' command.
 *
 * Fetches the man page for a given command from an online source (man.he.net)
 * and displays it to the user.
 *
 * @param command_name The name of the command to look up (e.g., "ls", "grep").
 */
void iman_execute(const char* command_name);

#endif // IMAN_H_