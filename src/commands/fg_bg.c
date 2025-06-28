#include "commands/fg_bg.h"
#include "utils/error.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

void fg_execute(pid_t pid, ShellState* state) {
    // A PID of 0 is invalid for fg/bg.
    if (pid <= 0) {
        print_shell_error("fg: Invalid PID provided.");
        return;
    }

    // Get the Process Group ID for the given PID. This is crucial for job control.
    pid_t job_pgid = getpgid(pid);
    printf("%d\n", job_pgid);
    if (job_pgid < 0) {
        print_shell_perror("fg: Could not find process with given PID");
        return;
    }

    // Find the job in our background list using its PGID.
    int job_index = -1;
    for (int i = 0; i < state->num_bg_processes; i++) {
        if (state->background_pids[i] == job_pgid) {
            job_index = i;
            break;
        }
    }

    if (job_index == -1) {
        print_shell_error("fg: Job is not a background process of this shell.");
        return;
    }

    // Save the job name before we remove it from the list.
    char job_name[MAX_PATH_LEN];
    strcpy(job_name, state->background_process_names[job_index]);

    // Remove the job from the background list *before* bringing it to the foreground.
    for (int i = job_index; i < state->num_bg_processes - 1; i++) {
        state->background_pids[i] = state->background_pids[i + 1];
        strcpy(state->background_process_names[i], state->background_process_names[i + 1]);
    }
    state->num_bg_processes--;

    // 1. Give terminal control to the job's process group.
    tcsetpgrp(STDIN_FILENO, job_pgid);

    // 2. Send SIGCONT to the entire process group to resume it.
    if (kill(-job_pgid, SIGCONT) < 0) {
        print_shell_perror("fg: Failed to send SIGCONT");
        // If we fail, we must take terminal control back.
        tcsetpgrp(STDIN_FILENO, getpgrp());
        return;
    }

    // 3. Wait for the now-foreground job to complete or be stopped again.
    int status;
    state->foreground_pgid = job_pgid;
    waitpid(-job_pgid, &status, WUNTRACED);
    state->foreground_pgid = -1;

    // 4. The shell MUST take back control of the terminal.
    tcsetpgrp(STDIN_FILENO, getpgrp());

    // 5. If the job was stopped again (by Ctrl+Z), add it back to the background list.
    if (WIFSTOPPED(status)) {
        printf("\nStopped: %s (PGID %d)\n", job_name, job_pgid);
        if (state->num_bg_processes < MAX_BG_PROCS) {
            state->background_pids[state->num_bg_processes] = job_pgid;
            strcpy(state->background_process_names[state->num_bg_processes], job_name);
            state->num_bg_processes++;
        }
    }
}

void bg_execute(pid_t pid, ShellState* state) {
    if (pid <= 0) {
        print_shell_error("bg: Invalid PID provided.");
        return;
    }

    pid_t job_pgid = getpgid(pid);
    if (job_pgid < 0) {
        print_shell_perror("bg: Could not find process with given PID");
        return;
    }

    bool job_found = false;
    for (int i = 0; i < state->num_bg_processes; i++) {
        if (state->background_pids[i] == job_pgid) {
            job_found = true;
            break;
        }
    }

    if (!job_found) {
        print_shell_error("bg: Job is not a background process of this shell.");
        return;
    }

    // Send SIGCONT to resume the job in the background.
    if (kill(-job_pgid, SIGCONT) < 0) {
        print_shell_perror("bg: Failed to send SIGCONT");
    }
}