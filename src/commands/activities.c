#include "commands/activities.h"
#include "utils/error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_process_stopped(pid_t pid) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE* f = fopen(path, "r");
    if (!f) {
        return false; // Process likely doesn't exist anymore
    }

    char state_char;
    // The state is the 3rd field in /proc/[pid]/stat
    // We use %*s to discard the fields we don't need.
    if (fscanf(f, "%*d %*s %c", &state_char) != 1) {
        fclose(f);
        return false; // Failed to read state
    }
    fclose(f);

    return (state_char == 'T');
}

void activities_execute(const ShellState* state) {
    if (state->num_bg_processes == 0) {
        printf("No background activities.\n");
        return;
    }

    printf("Background Activities:\n");

    // The source of truth is our shell's internal state.
    // We iterate through the list of background jobs we are tracking.
    for (int i = 0; i < state->num_bg_processes; i++) {
        pid_t job_pgid = state->background_pids[i];
        const char* job_name = state->background_process_names[i];
        const char* display_state;

        // A job's state is determined by the state of its leader process (pgid).
        if (is_process_stopped(job_pgid)) {
            display_state = "Stopped";
        } else {
            display_state = "Running";
        }

        // Print the information for the job. We use the PGID as the job identifier.
        printf("%d: %s - %s\n", job_pgid, job_name, display_state);
    }
}