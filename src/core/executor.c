#include "core/executor.h"
#include "core/parser.h"
#include "utils/error.h"
#include "commands/warp.h"
#include "commands/peek.h"
#include "commands/proclore.h"
#include "commands/seek.h"
#include "commands/iman.h"
#include "commands/activities.h"
#include "commands/ping.h"
#include "commands/neonate.h"
#include "commands/fg_bg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>

// Forward declarations for internal functions
static void execute_pipeline(SimpleCommand commands[], int num_commands, bool is_background, ShellState* state);
static void check_background_processes(ShellState* state);
static void execute_builtin_command(SimpleCommand* cmd, ShellState* state);
static bool is_builtin_command(const char* cmd_name);

// This is the main entry point from the main loop
void process_input_line(char* input_line, ShellState* state) {
    check_background_processes(state);

    char original_input_for_history[MAX_INPUT_LEN];
    strncpy(original_input_for_history, input_line, sizeof(original_input_for_history) - 1);
    original_input_for_history[sizeof(original_input_for_history) - 1] = '\0';

    bool add_original_to_history = true;

    char* sc_saveptr;
    char* current_sc_command_str = strtok_r(input_line, ";", &sc_saveptr);

    while (current_sc_command_str != NULL) {
        char command_to_process[MAX_COMMAND_LEN];
        strncpy(command_to_process, current_sc_command_str, sizeof(command_to_process) - 1);
        command_to_process[sizeof(command_to_process) - 1] = '\0';

        // Trim whitespace
        char* trimmed_cmd = command_to_process;
        while (isspace((unsigned char)*trimmed_cmd)) trimmed_cmd++;
        if (*trimmed_cmd == '\0') {
            current_sc_command_str = strtok_r(NULL, ";", &sc_saveptr);
            continue;
        }
        char* end = trimmed_cmd + strlen(trimmed_cmd) - 1;
        while (end > trimmed_cmd && isspace((unsigned char)*end)) end--;
        *(end + 1) = '\0';

        // Handle 'pastevents execute' before any other parsing
        if (strncmp(trimmed_cmd, "pastevents execute", 18) == 0) {
            char temp_cmd[MAX_COMMAND_LEN];
            strcpy(temp_cmd, trimmed_cmd);
            char* pe_saveptr;
            strtok_r(temp_cmd, " \t", &pe_saveptr); // "pastevents"
            strtok_r(NULL, " \t", &pe_saveptr);   // "execute"
            char* num_str = strtok_r(NULL, " \t", &pe_saveptr);
            if (num_str) {
                int k = atoi(num_str);
                char* hist_cmd = get_kth_history_element(state->history_queue, k);
                if (hist_cmd) {
                    strcpy(trimmed_cmd, hist_cmd); // Overwrite with command from history
                    add_history_element(state->history_queue, hist_cmd);
                    add_original_to_history = false;
                    free(hist_cmd);
                } else {
                    current_sc_command_str = strtok_r(NULL, ";", &sc_saveptr);
                    continue; // Skip this invalid command
                }
            } else {
                print_shell_error("pastevents execute: Number not provided.");
                current_sc_command_str = strtok_r(NULL, ";", &sc_saveptr);
                continue;
            }
        }

        long start_time = time(NULL);

        SimpleCommand commands[MAX_ARGS];
        bool is_background = false;
        int num_commands = parse_pipeline(trimmed_cmd, commands, MAX_ARGS, &is_background);

        if (num_commands > 0) {
            // Set command name for prompt
            strncpy(state->last_command_name, commands[0].args[0], MAX_COMMAND_LEN - 1);
            state->last_command_name[MAX_COMMAND_LEN - 1] = '\0';

            if (num_commands == 1 && is_builtin_command(commands[0].args[0])) {
                execute_builtin_command(&commands[0], state);
            } else {
                execute_pipeline(commands, num_commands, is_background, state);
            }
            free_simple_commands(commands, num_commands);
        } else if (num_commands == -1) {
            // Parsing error already printed by parser
        }

        state->time_taken_for_prompt = time(NULL) - start_time;
        current_sc_command_str = strtok_r(NULL, ";", &sc_saveptr);
    }

    if (add_original_to_history && strlen(original_input_for_history) > 0) {
        add_history_element(state->history_queue, original_input_for_history);
    }
}

static bool is_builtin_command(const char* cmd_name) {
    if (!cmd_name) return false;
    const char* builtins[] = {"q", "quit", "exit", "warp", "peek", "pastevents", "proclore", "seek", "iman", "activities", "ping", "neonate", "fg", "bg", NULL};
    for (int i = 0; builtins[i] != NULL; i++) {
        if (strcmp(cmd_name, builtins[i]) == 0) {
            return true;
        }
    }
    return false;
}

static void execute_builtin_command(SimpleCommand* cmd, ShellState* state) {
    int argc = 0;
    while(cmd->args[argc] != NULL) argc++;
    char* cmd_name = cmd->args[0];

    if (strcmp(cmd_name, "q") == 0 || strcmp(cmd_name, "quit") == 0 || strcmp(cmd_name, "exit") == 0) {
        state->is_running = false;
    } else if (strcmp(cmd_name, "warp") == 0) {
        if (argc < 2) {
            char* result = warp("~", state->home_dir, state->prev_dir);
            if (result) { strncpy(state->prev_dir, result, sizeof(state->prev_dir) - 1); free(result); }
        } else {
            for (int j = 1; j < argc; j++) {
                char* result = warp(cmd->args[j], state->home_dir, state->prev_dir);
                if (result) { strncpy(state->prev_dir, result, sizeof(state->prev_dir) - 1); free(result); }
            }
        }
    } else if (strcmp(cmd_name, "peek") == 0) {
        bool l = false, a = false; char* path = NULL;
        for(int i=1; i<argc; ++i) {
            if(cmd->args[i][0] == '-') for(size_t j=1; j<strlen(cmd->args[i]); ++j) { if(cmd->args[i][j]=='l') l=true; else if(cmd->args[i][j]=='a') a=true; }
            else path = cmd->args[i];
        }
        peek_execute(path, state->home_dir, l, a);
    } else if (strcmp(cmd_name, "pastevents") == 0) {
        if (argc == 1) display_history(state->history_queue);
        else if (argc == 2 && strcmp(cmd->args[1], "purge") == 0) {
            purge_history(state->history_queue);
            write_history_to_file(state->history_queue, state->home_dir);
        } else print_shell_error("pastevents: Invalid arguments.");
    } else if (strcmp(cmd_name, "proclore") == 0) {
        proclore_execute(argc > 1 ? atoi(cmd->args[1]) : getpid(), state->home_dir);
    } else if (strcmp(cmd_name, "seek") == 0) {
        bool d=false, f=false, e=false; char* name=NULL; char* dir="."; int i=1;
        while(i < argc && cmd->args[i][0] == '-') {
            for(size_t j=1; j<strlen(cmd->args[i]); ++j) { if(cmd->args[i][j]=='d') d=true; else if(cmd->args[i][j]=='f') f=true; else if(cmd->args[i][j]=='e') e=true; }
            i++;
        }
        if(i < argc) name = cmd->args[i++];
        if(i < argc) dir = cmd->args[i];
        if(!name) { print_shell_error("seek: Target name not specified."); return; }
        if(d && f) { print_shell_error("seek: Flags -d and -f are mutually exclusive."); return; }
        seek_execute(name, dir, state->home_dir, state->prev_dir, d, f, e);
    } else if (strcmp(cmd_name, "iman") == 0) {
        if (argc != 2) {
            print_shell_error("Usage: iman <command_name>");
        } else {
            iman_execute(cmd->args[1]);
        }
    } else if (strcmp(cmd_name, "activities") == 0) {
        if (argc != 1) {
            print_shell_error("Usage: activities (takes no arguments)");
        } else {
            activities_execute(state);
        }
    } else if (strcmp(cmd_name, "ping") == 0) {
        if (argc != 3) {
            print_shell_error("Usage: ping <pid> <signal_number>");
        } else {
            pid_t target_pid = atoi(cmd->args[1]);
            int signal_num = atoi(cmd->args[2]);
            ping_execute(target_pid, signal_num);
        }
    } else if (strcmp(cmd_name, "neonate") == 0) {
        if (argc != 3 || strcmp(cmd->args[1], "-n") != 0) {
            print_shell_error("Usage: neonate -n <time_in_seconds>");
        } else {
            int time_arg = atoi(cmd->args[2]);
            if (time_arg <= 0) {
                print_shell_error("neonate: time argument must be a positive integer.");
            } else {
                neonate_execute(time_arg);
            }
        }
    } else if (strcmp(cmd_name, "bg") == 0) {
        if (argc != 2) {
            print_shell_error("Usage: bg <pid>");
        } else {
            pid_t target_pid = atoi(cmd->args[1]);
            bg_execute(target_pid, state);
        }
    } else if (strcmp(cmd_name, "fg") == 0) {
        if (argc != 2) {
            print_shell_error("Usage: fg <pid>");
        } else {
            pid_t target_pid = atoi(cmd->args[1]);
            fg_execute(target_pid, state);
        }
    }
}

static void execute_pipeline(SimpleCommand commands[], int num_commands, bool is_background, ShellState* state) {
    int input_fd = STDIN_FILENO;
    int pipe_fds[2];
    pid_t pids[num_commands];
    pid_t pgid = 0; // The process group ID for this pipeline

    for (int i = 0; i < num_commands; i++) {
        if (i < num_commands - 1) {
            if (pipe(pipe_fds) < 0) { print_shell_perror("pipe failed"); return; }
        }
        pids[i] = fork();
        if (pids[i] < 0) { print_shell_perror("fork failed"); return; }

        if (pids[i] == 0) { // --- Child Process ---
            // Set the process group ID. The first child sets it for the whole pipeline.
            pgid = (i == 0) ? getpid() : pgid;
            setpgid(0, pgid);

            if (!is_background) {
                // If it's a foreground job, it needs terminal control.
                tcsetpgrp(STDIN_FILENO, pgid);
            }
            // A child process should not ignore signals. Reset to default handlers.
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            // --- I/O Redirection Logic (unchanged) ---
            if (i > 0) { dup2(input_fd, STDIN_FILENO); close(input_fd); }
            if (commands[i].input_file) {
                int in_fd = open(commands[i].input_file, O_RDONLY);
                if (in_fd < 0) { print_shell_perror(commands[i].input_file); exit(EXIT_FAILURE); }
                dup2(in_fd, STDIN_FILENO); close(in_fd);
            }
            if (i < num_commands - 1) {
                dup2(pipe_fds[1], STDOUT_FILENO);
                close(pipe_fds[0]); close(pipe_fds[1]);
            }
            if (commands[i].output_file) {
                int flags = O_WRONLY | O_CREAT | (commands[i].append_mode ? O_APPEND : O_TRUNC);
                int out_fd = open(commands[i].output_file, flags, 0644);
                if (out_fd < 0) { print_shell_perror(commands[i].output_file); exit(EXIT_FAILURE); }
                dup2(out_fd, STDOUT_FILENO); close(out_fd);
            }
            // --- Execution (unchanged) ---
            if (execvp(commands[i].args[0], commands[i].args) == -1) {
                fprintf(stderr, _RED_ "Shell Error: Command '%s' not found" _RESET_ "\n", commands[i].args[0]);
                exit(EXIT_FAILURE);
            }
        }
        // --- Parent Process ---
        // The first child's PID becomes the PGID for the whole group.
        if (i == 0) {
            pgid = pids[0];
        }
        setpgid(pids[i], pgid); // Set PGID for all children in the pipeline

        if (input_fd != STDIN_FILENO) close(input_fd);
        if (i < num_commands - 1) { close(pipe_fds[1]); input_fd = pipe_fds[0]; }
    }

    // --- Parent Process Waits or Continues ---
    if (!is_background) {
        // --- FOREGROUND JOB ---
        state->foreground_pgid = pgid; // Set global state
        tcsetpgrp(STDIN_FILENO, pgid); // Give terminal control to the child group

        // Wait for all processes in the pipeline to finish or be stopped
        for (int i = 0; i < num_commands; i++) {
            int status;
            // WUNTRACED is crucial for catching Ctrl+Z (SIGTSTP)
            waitpid(pids[i], &status, WUNTRACED);

            if (WIFSTOPPED(status)) {
                // Process was stopped by Ctrl+Z
                printf("\nStopped: %s (PID %d)\n", commands[i].args[0], pids[i]);
                // Add the entire pipeline to background jobs
                if (state->num_bg_processes < MAX_BG_PROCS) {
                    state->background_pids[state->num_bg_processes] = pgid; // Track by PGID
                    strncpy(state->background_process_names[state->num_bg_processes], commands[0].args[0], MAX_PATH_LEN - 1);
                    state->num_bg_processes++;
                }
                break; // Stop waiting for other processes in the pipeline
            }
        }

        tcsetpgrp(STDIN_FILENO, getpgrp()); // Take back terminal control
        state->foreground_pgid = -1; // Reset global state
    } else {
        // --- BACKGROUND JOB ---
        if (state->num_bg_processes < MAX_BG_PROCS) {
            state->background_pids[state->num_bg_processes] = pgid; // Track by PGID
            strncpy(state->background_process_names[state->num_bg_processes], commands[0].args[0], MAX_PATH_LEN - 1);
            state->num_bg_processes++;
            printf("Shell: Started background job [%d] %s (PGID %d)\n", state->num_bg_processes, commands[0].args[0], pgid);
        } else {
            print_shell_error("Maximum background processes reached.");
            kill(-pgid, SIGKILL); // Kill the job if we can't track it
        }
    }
}

static void check_background_processes(ShellState* state) {
    int current_valid_idx = 0;
    for (int i = 0; i < state->num_bg_processes; i++) {
        int status;
        // CRITICAL FIX: Use -pgid to wait for any process in the group.
        // WNOHANG makes the call non-blocking.
        pid_t result = waitpid(-state->background_pids[i], &status, WNOHANG);

        if (result > 0) { // A process in the group terminated.
            // We consider the entire job terminated if one of its processes exits.
            // A more complex shell might wait for all, but this is a robust simplification.
            printf("\nShell: Background job '%s' (PGID %d) has terminated.\n",
                   state->background_process_names[i], state->background_pids[i]);
            // Do not copy this job to the compacted list, effectively removing it.
        } else if (result == 0) { // No change in the process group, job is still running.
            if (i != current_valid_idx) {
                state->background_pids[current_valid_idx] = state->background_pids[i];
                strcpy(state->background_process_names[current_valid_idx], state->background_process_names[i]);
            }
            current_valid_idx++;
        } else { // An error occurred, or the process group no longer exists.
            if (errno != ECHILD) {
                // This case is rare, but we should handle it.
                // We assume the job is gone and remove it.
            }
        }
    }
    state->num_bg_processes = current_valid_idx;
}