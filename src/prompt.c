#include "shell_headers.h"

/**
 * @brief Displays the shell prompt.
 *
 * The prompt includes username, system name, current directory (relative to home_dir if applicable),
 * and optionally the execution time of the last command if it exceeded 1 second.
 * @param home_dir The home directory path of the shell user.
 * @param last_command_name The name of the last executed command. Can be NULL or empty.
 * @param time_taken Time taken by the last command in seconds.
 */
void display_shell_prompt(const char* home_dir, const char* last_command_name, long time_taken) {
    struct utsname sys_info;
    if (uname(&sys_info) != 0) {
        // Fallback if uname fails
        strncpy(sys_info.nodename, "localhost", sizeof(sys_info.nodename) - 1);
        sys_info.nodename[sizeof(sys_info.nodename) - 1] = '\0';
        // No perror here to keep prompt clean, error handled if critical elsewhere
    }

    char* username = getlogin();
    if (!username) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            username = pw->pw_name;
        } else {
            username = "user"; // Fallback username
        }
    }

    char current_path[MAX_PATH_LEN];
    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        strcpy(current_path, "?"); // Fallback path
    }

    char display_path[MAX_PATH_LEN];
    size_t home_dir_len = strlen(home_dir);
    if (strncmp(current_path, home_dir, home_dir_len) == 0) {
        if (strlen(current_path) == home_dir_len) { // Exactly home_dir
            strcpy(display_path, "~");
        } else if (current_path[home_dir_len] == '/') { // home_dir/something
            snprintf(display_path, sizeof(display_path), "~%s", current_path + home_dir_len);
        } else { // e.g. /home/userX vs /home/user (current_path is longer but not a subpath starting with /)
            strncpy(display_path, current_path, sizeof(display_path) - 1);
            display_path[sizeof(display_path) - 1] = '\0';
        }
    } else {
        strncpy(display_path, current_path, sizeof(display_path) - 1);
        display_path[sizeof(display_path) - 1] = '\0';
    }

    printf(_GREEN_ "<" _RESET_ _BLUE_ "%s" _RESET_ _GREEN_ "@" _RESET_ "%s:" _MAGENTA_ "%s" _RESET_,
           username, sys_info.nodename, display_path);

    if (last_command_name && strlen(last_command_name) > 0 && time_taken > 1) {
        printf(" %s: %lds", last_command_name, time_taken);
    }
    printf(_GREEN_ "> " _RESET_);
    fflush(stdout); // Ensure prompt is displayed before fgets
}