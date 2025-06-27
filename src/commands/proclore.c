#include "commands/proclore.h"
#include "core/shell_state.h"
#include "utils/error.h" 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

void proclore_execute(int pid, const char* home_dir) {
    char proc_status_path[MAX_PATH_LEN];
    snprintf(proc_status_path, sizeof(proc_status_path), "/proc/%d/status", pid);

    FILE* f_status = fopen(proc_status_path, "r");
    if (!f_status) {
        fprintf(stderr, _RED_ "Shell Error: " _RESET_ "proclore: Could not open %s: %s\n",
                proc_status_path, strerror(errno));
        return;
    }

    printf(_BLUE_ "pid : " _RESET_ "%d\n", pid);

    char line_buffer[256];
    char process_state[32] = "N/A";
    char vm_size[32] = "N/A";
    pid_t process_group_id = -1;

    while (fgets(line_buffer, sizeof(line_buffer), f_status)) {
        if (strncmp(line_buffer, "State:", 6) == 0) {
            sscanf(line_buffer, "State:\t%s", process_state);
        } else if (strncmp(line_buffer, "VmSize:", 7) == 0) {
            sscanf(line_buffer, "VmSize:\t%s kB", vm_size); // VmSize is usually in kB
        } else if (strncmp(line_buffer, "PPid:", 5) == 0) { // Example: could also get PGid from /proc/[pid]/stat
            // For process group, it's better to use getpgid()
        }
    }
    fclose(f_status);

    process_group_id = getpgid(pid);
    if (process_group_id == -1) {
        // Error getting pgid, but continue if other info was found
        // print_shell_perror("proclore: getpgid failed");
    }

    // Determine if foreground (+)
    // A process is foreground if its process group ID is the same as the
    // foreground process group ID of the controlling terminal.
    pid_t terminal_pgid = tcgetpgrp(STDIN_FILENO); // Get foreground process group of terminal
    
    printf(_BLUE_ "Process State : " _RESET_ "%s%s\n", process_state,
           (terminal_pgid != -1 && process_group_id == terminal_pgid) ? "+" : "");
    printf(_BLUE_ "Process Group : " _RESET_ "%d\n", process_group_id);
    printf(_BLUE_ "Virtual Memory : " _RESET_ "%s kB\n", vm_size);


    char proc_exe_path[MAX_PATH_LEN];
    snprintf(proc_exe_path, sizeof(proc_exe_path), "/proc/%d/exe", pid);
    char executable_path[MAX_PATH_LEN] = {0}; // Initialize to empty
    ssize_t len = readlink(proc_exe_path, executable_path, sizeof(executable_path) - 1);

    if (len != -1) {
        executable_path[len] = '\0';
        char display_exe_path[MAX_PATH_LEN];
        size_t home_dir_len = strlen(home_dir);

        if (strncmp(executable_path, home_dir, home_dir_len) == 0 &&
            (executable_path[home_dir_len] == '/' || executable_path[home_dir_len] == '\0')) {
            snprintf(display_exe_path, sizeof(display_exe_path), "~%s", executable_path + home_dir_len);
        } else {
            strncpy(display_exe_path, executable_path, sizeof(display_exe_path) - 1);
            display_exe_path[sizeof(display_exe_path) -1] = '\0';
        }
        printf(_BLUE_ "Executable Path : " _RESET_ "%s\n", display_exe_path);
    } else {
        // readlink can fail if process is a zombie or due to permissions
        // print_shell_perror("proclore: readlink for executable path failed");
        printf(_BLUE_ "Executable Path : " _RESET_ "[Permission Denied or Path Not Found]\n");
    }
}