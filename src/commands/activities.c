#include "commands/activities.h"
#include "utils/error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

/**
 * @brief A filter function for scandir to select only directories whose names are PIDs.
 * @param entry The directory entry to check.
 * @return 1 if the entry name is a number, 0 otherwise.
 */
static int filter_pids(const struct dirent *entry) {
    for (int i = 0; entry->d_name[i]; i++) {
        if (!isdigit(entry->d_name[i])) {
            return 0; // Not a valid PID directory
        }
    }
    return 1;
}

void activities_execute(const ShellState* state) {
    struct dirent **namelist;
    int count;

    // Scan /proc for all numerical directories, sorted by name (PID)
    count = scandir("/proc", &namelist, filter_pids, alphasort);
    if (count < 0) {
        print_shell_perror("activities: Failed to scan /proc");
        return;
    }

    pid_t shell_pgid = getpgid(getpid());

    for (int i = 0; i < count; ++i) {
        pid_t pid = atoi(namelist[i]->d_name);

        // A process belongs to our shell if its process group ID is the same as the shell's.
        // We also exclude the shell process itself.
        if (getpgid(pid) == shell_pgid && pid != getpid()) {
            char path_buffer[MAX_PATH_LEN];
            char line_buffer[256];
            char process_name[MAX_COMMAND_LEN] = "N/A";
            char process_state_char[32] = "N/A";

            // 1. Get the process name from /proc/[pid]/cmdline
            snprintf(path_buffer, sizeof(path_buffer), "/proc/%d/cmdline", pid);
            FILE *f_cmd = fopen(path_buffer, "r");
            if (f_cmd) {
                if (fgets(process_name, sizeof(process_name), f_cmd) == NULL) {
                    // If cmdline is empty (e.g., for a zombie), set a default
                    strcpy(process_name, "N/A");
                }
                fclose(f_cmd);
            }

            // Fallback for zombie processes that have no cmdline but are in our bg list
            if (strcmp(process_name, "N/A") == 0) {
                for(int j = 0; j < state->num_bg_processes; j++) {
                    if(state->background_pids[j] == pid) {
                        strncpy(process_name, state->background_process_names[j], sizeof(process_name) - 1);
                        process_name[sizeof(process_name) - 1] = '\0';
                        break;
                    }
                }
            }

            // 2. Get the process state from /proc/[pid]/status
            snprintf(path_buffer, sizeof(path_buffer), "/proc/%d/status", pid);
            FILE *f_stat = fopen(path_buffer, "r");
            if (f_stat) {
                while (fgets(line_buffer, sizeof(line_buffer), f_stat)) {
                    if (strncmp(line_buffer, "State:", 6) == 0) {
                        sscanf(line_buffer, "State:\t%s", process_state_char);
                        break;
                    }
                }
                fclose(f_stat);
            }

            // 3. Determine display state and print
            const char* display_state = "Running";
            if (process_state_char[0] == 'T' || process_state_char[0] == 'Z') {
                display_state = "Stopped";
            }

            printf("%d: %s - %s\n", pid, process_name, display_state);
        }
        free(namelist[i]);
    }
    free(namelist);
}