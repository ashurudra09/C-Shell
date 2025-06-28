#include "core/shell_state.h"
#include "utils/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>

bool shell_state_init(ShellState* state) {
    if (getcwd(state->home_dir, sizeof(state->home_dir)) == NULL) {
        print_shell_perror("Failed to get initial working directory");
        return false;
    }

    state->prev_dir[0] = '\0';
    state->history_queue = initQue();
    read_history_from_file(state->history_queue, state->home_dir);

    state->is_running = true;
    state->num_bg_processes = 0;
    state->last_command_name[0] = '\0';
    state->time_taken_for_prompt = -1;
    state->foreground_pgid = -1;
    
    // Initialize background process arrays to be safe
    for (int i = 0; i < MAX_BG_PROCS; i++) {
        state->background_pids[i] = -1;
        state->background_process_names[i][0] = '\0';
    }

    return true;
}

void shell_state_destroy(ShellState* state) {
    write_history_to_file(state->history_queue, state->home_dir);
    destroyQue(state->history_queue);
    state->history_queue = NULL;
}

// This function is the former display_shell_prompt from prompt.c
void display_shell_prompt(const ShellState* state) {
    struct utsname sys_info;
    if (uname(&sys_info) != 0) {
        strncpy(sys_info.nodename, "localhost", sizeof(sys_info.nodename) - 1);
        sys_info.nodename[sizeof(sys_info.nodename) - 1] = '\0';
    }

    char* username = getlogin();
    if (!username) {
        struct passwd *pw = getpwuid(getuid());
        username = pw ? pw->pw_name : "user";
    }

    char current_path[MAX_PATH_LEN];
    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        strcpy(current_path, "?");
    }

    char display_path[MAX_PATH_LEN];
    size_t home_dir_len = strlen(state->home_dir);
    if (strncmp(current_path, state->home_dir, home_dir_len) == 0 &&
        (strlen(current_path) == home_dir_len || current_path[home_dir_len] == '/')) {
        snprintf(display_path, sizeof(display_path), "~%s", current_path + home_dir_len);
    } else {
        strncpy(display_path, current_path, sizeof(display_path) - 1);
        display_path[sizeof(display_path) - 1] = '\0';
    }

    printf(_GREEN_ "<" _RESET_ _BLUE_ "%s" _RESET_ _GREEN_ "@" _RESET_ "%s:" _MAGENTA_ "%s" _RESET_,
           username, sys_info.nodename, display_path);

    if (strlen(state->last_command_name) > 0 && state->time_taken_for_prompt > 1) {
        printf(" %s: %lds", state->last_command_name, state->time_taken_for_prompt);
    }
    printf(_GREEN_ "> " _RESET_);
    fflush(stdout);
}