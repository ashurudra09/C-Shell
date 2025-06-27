#ifndef SHELL_STATE_H_
#define SHELL_STATE_H_

#include "utils/que.h"
#include "utils/colors.h" // <-- ADD THIS
#include <stdbool.h>
#include <sys/types.h>

// Common buffer sizes and limits
#define MAX_PATH_LEN 4096
#define MAX_INPUT_LEN 4096
#define MAX_COMMAND_LEN 4096
#define MAX_ARGS 64
#define MAX_BG_PROCS 100
#define HISTORY_SIZE 15
#define HISTORY_FILENAME ".shellby_history.txt"

/*
 * REMOVE the old color code definitions from here.
 * They are now in utils/colors.h
 */

/**
 * @brief Represents a single command with its arguments and I/O redirection info.
 */
typedef struct {
    char* args[MAX_ARGS];
    char* input_file;
    char* output_file;
    bool append_mode;
} SimpleCommand;

/**
 * @brief Holds all persistent state for the shell instance.
 */
typedef struct {
    char home_dir[MAX_PATH_LEN];
    char prev_dir[MAX_PATH_LEN];
    Que history_queue;
    bool is_running;

    // Background process tracking
    int background_pids[MAX_BG_PROCS];
    char background_process_names[MAX_BG_PROCS][MAX_PATH_LEN];
    int num_bg_processes;

    // For prompt display
    char last_command_name[MAX_COMMAND_LEN];
    long time_taken_for_prompt;

} ShellState;

/**
 * @brief Initializes the shell state.
 * @param state Pointer to the ShellState struct to initialize.
 * @return true on success, false on critical failure.
 */
bool shell_state_init(ShellState* state);

/**
 * @brief Frees resources held by the shell state.
 * @param state Pointer to the ShellState struct to clean up.
 */
void shell_state_destroy(ShellState* state);

/**
 * @brief Displays the shell prompt using info from the shell state.
 * @param state The current state of the shell.
 */
void display_shell_prompt(const ShellState* state);

#endif // SHELL_STATE_H_