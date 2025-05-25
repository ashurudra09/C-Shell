#include "shell_headers.h"

/**
 * @brief Prints a formatted error message to stderr.
 * @param message The error message string.
 */
void print_shell_error(const char* message) {
    fprintf(stderr, _RED_ "Shell Error: " _RESET_ "%s\n", message);
}

/**
 * @brief Prints a formatted error message with perror style to stderr.
 * @param message The custom part of the error message.
 */
void print_shell_perror(const char* message) {
    fprintf(stderr, _RED_ "Shell Error: " _RESET_ "%s: %s\n", message, strerror(errno));
}


/**
 * @brief Counts occurrences of a character in a string.
 * @param str The string to search in.
 * @param c The character to count.
 * @return The number of occurrences, or 0 if str is NULL.
 */
int count_char_occurrences(const char* str, char c) {
    int count = 0;
    if (!str) {
        return 0;
    }
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == c) {
            count++;
        }
    }
    return count;
}

/**
 * @brief Handles completed background processes, prints their status, and cleans up the list.
 * @param background_pids Array of background process PIDs.
 * @param bg_process_names Array of background process names.
 * @param num_bg_processes Pointer to the current number of background processes.
 */
void check_background_processes(int background_pids[], char bg_process_names[][MAX_PATH_LEN], int* num_bg_processes) {
    int current_valid_idx = 0;
    for (int i = 0; i < *num_bg_processes; i++) {
        int status;
        pid_t result = waitpid(background_pids[i], &status, WNOHANG);

        if (result == background_pids[i]) { // Child terminated or changed state
            if (WIFEXITED(status)) {
                printf("Shell: Background process '%s' (PID %d) exited normally with status %d.\n",
                       bg_process_names[i], background_pids[i], WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Shell: Background process '%s' (PID %d) exited abnormally due to signal %d (%s).\n",
                       bg_process_names[i], background_pids[i], WTERMSIG(status), strsignal(WTERMSIG(status)));
            }
            // Don't copy this process to the compacted list
        } else if (result == 0) { // Child still running
            if (i != current_valid_idx) {
                background_pids[current_valid_idx] = background_pids[i];
                strncpy(bg_process_names[current_valid_idx], bg_process_names[i], MAX_PATH_LEN - 1);
                bg_process_names[current_valid_idx][MAX_PATH_LEN - 1] = '\0';
            }
            current_valid_idx++;
        } else if (result == -1) {
            if (errno != ECHILD) { // ECHILD means child already reaped or never existed
                 print_shell_perror("waitpid error");
            }
            // In any case of error or ECHILD, this process is no longer tracked or valid.
        }
    }
    *num_bg_processes = current_valid_idx;
}

/**
 * @brief Parses arguments from a command string. Modifies command_str.
 * @param command_str The command string to parse (will be modified by strtok).
 * @param args Array to store argument pointers.
 * @param max_args Maximum number of arguments to parse.
 * @param is_background Pointer to a boolean flag, set if '&' is the last token.
 * @return The number of arguments (argc).
 */
int parse_arguments(char* command_str, char* args[], int max_args, bool* is_background) {
    int argc = 0;
    char* token;
    *is_background = false;

    token = strtok(command_str, " \t\n\r"); // Delimiters: space, tab, newline, carriage return
    while (token != NULL && argc < max_args - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n\r");
    }
    args[argc] = NULL; // Null-terminate the argument list for execvp

    if (argc > 0 && strcmp(args[argc - 1], "&") == 0) {
        *is_background = true;
        args[argc - 1] = NULL; // Remove '&' from arguments
        argc--;
    }
    return argc;
}

/**
 * @brief Main function for the shell.
 * @return EXIT_SUCCESS on normal termination, EXIT_FAILURE on critical error.
 */
int main() {
    char home_dir[MAX_PATH_LEN];
    if (getcwd(home_dir, sizeof(home_dir)) == NULL) {
        print_shell_perror("Failed to get initial working directory (home_dir)");
        return EXIT_FAILURE;
    }

    char prev_dir[MAX_PATH_LEN] = "";
    Que history_queue = initQue();
    read_history_from_file(history_queue, home_dir);

    char input_line[MAX_INPUT_LEN];
    char original_input_for_history[MAX_INPUT_LEN];

    char** semi_colon_commands = NULL;
    long int time_taken_for_prompt = -1;
    char command_name_for_prompt[MAX_COMMAND_LEN] = "";

    int background_pids[MAX_BG_PROCS];
    char background_process_names[MAX_BG_PROCS][MAX_PATH_LEN];
    int num_bg_processes = 0;

    while (1) {
        check_background_processes(background_pids, background_process_names, &num_bg_processes);

        display_shell_prompt(home_dir, command_name_for_prompt, time_taken_for_prompt);
        command_name_for_prompt[0] = '\0';
        time_taken_for_prompt = -1;

        if (fgets(input_line, sizeof(input_line), stdin) == NULL) {
            if (feof(stdin)) { // EOF (Ctrl+D)
                printf("\nGoodbye!\n");
                break;
            } else { // Some other read error
                print_shell_perror("fgets failed");
                break; // Exit on read error
            }
        }

        strncpy(original_input_for_history, input_line, sizeof(original_input_for_history) - 1);
        original_input_for_history[sizeof(original_input_for_history) - 1] = '\0';
        size_t len_hist_copy = strlen(original_input_for_history);
        if (len_hist_copy > 0 && original_input_for_history[len_hist_copy - 1] == '\n') {
            original_input_for_history[len_hist_copy - 1] = '\0';
        }

        char mutable_input_line[MAX_INPUT_LEN];
        strncpy(mutable_input_line, input_line, sizeof(mutable_input_line) -1);
        mutable_input_line[sizeof(mutable_input_line)-1] = '\0';
        
        size_t len_input = strlen(mutable_input_line);
        if (len_input > 0 && mutable_input_line[len_input - 1] == '\n') {
            mutable_input_line[len_input - 1] = '\0';
        }
        if (strlen(mutable_input_line) == 0) continue;


        int num_sc_commands_expected = count_char_occurrences(mutable_input_line, ';') + 1;
        semi_colon_commands = (char**)malloc(sizeof(char*) * num_sc_commands_expected);
        if (!semi_colon_commands) {
            print_shell_perror("malloc for semi_colon_commands failed");
            continue;
        }

        char* current_sc_command_str = strtok(mutable_input_line, ";");
        int actual_num_sc_commands = 0;
        while (current_sc_command_str != NULL && actual_num_sc_commands < num_sc_commands_expected) {
            semi_colon_commands[actual_num_sc_commands++] = current_sc_command_str;
            current_sc_command_str = strtok(NULL, ";");
        }

        bool add_original_to_history = true;

        for (int i = 0; i < actual_num_sc_commands; i++) {
            char command_to_process_storage[MAX_COMMAND_LEN];
            strncpy(command_to_process_storage, semi_colon_commands[i], sizeof(command_to_process_storage)-1);
            command_to_process_storage[sizeof(command_to_process_storage)-1] = '\0';

            char* trimmed_cmd = command_to_process_storage;
            while (isspace((unsigned char)*trimmed_cmd)) trimmed_cmd++;
            char* end = trimmed_cmd + strlen(trimmed_cmd) - 1;
            while (end > trimmed_cmd && isspace((unsigned char)*end)) end--;
            *(end + 1) = '\0';

            if (strlen(trimmed_cmd) == 0) continue;

            // For prompt display: get first word of the potentially substituted command
            char temp_for_prompt_name[MAX_COMMAND_LEN];
            strncpy(temp_for_prompt_name, trimmed_cmd, sizeof(temp_for_prompt_name)-1);
            temp_for_prompt_name[sizeof(temp_for_prompt_name)-1] = '\0';
            char *first_word = strtok(temp_for_prompt_name, " \t\n\r");
            if (first_word) {
                strncpy(command_name_for_prompt, first_word, MAX_COMMAND_LEN -1);
                command_name_for_prompt[MAX_COMMAND_LEN-1] = '\0';
            } else {
                command_name_for_prompt[0] = '\0';
            }


            long start_time = time(NULL);
            bool is_background_job = false;
            char* args[MAX_ARGS];
            
            // Handle pastevents execute specifically, as it modifies the command to be run
            if (strncmp(trimmed_cmd, "pastevents", 10) == 0) {
                char temp_pastevents_cmd[MAX_COMMAND_LEN];
                strncpy(temp_pastevents_cmd, trimmed_cmd, sizeof(temp_pastevents_cmd)-1);
                temp_pastevents_cmd[sizeof(temp_pastevents_cmd)-1] = '\0';

                char* pe_saveptr;
                char* pe_token = strtok_r(temp_pastevents_cmd, " \t\n\r", &pe_saveptr); // "pastevents"
                if (pe_token) {
                    pe_token = strtok_r(NULL, " \t\n\r", &pe_saveptr); // "execute" or "purge" or NULL
                    if (pe_token && strcmp(pe_token, "execute") == 0) {
                        pe_token = strtok_r(NULL, " \t\n\r", &pe_saveptr); // The number k
                        if (!pe_token) {
                            print_shell_error("pastevents execute: Number not given");
                            time_taken_for_prompt = time(NULL) - start_time;
                            continue;
                        }
                        int k = atoi(pe_token);
                        if (k <= 0) {
                            print_shell_error("pastevents execute: Invalid number for k");
                            time_taken_for_prompt = time(NULL) - start_time;
                            continue;
                        }
                        char* command_from_history = get_kth_history_element(history_queue, k);
                        if (command_from_history) {
                            strncpy(trimmed_cmd, command_from_history, MAX_COMMAND_LEN-1); // command_to_process_storage gets new content
                            trimmed_cmd[MAX_COMMAND_LEN-1] = '\0';
                            add_history_element(history_queue, command_from_history); // Add executed command to history
                            write_history_to_file(history_queue, home_dir);
                            add_original_to_history = false; // Don't add "pastevents execute ..." itself
                            free(command_from_history);
                            // Update command_name_for_prompt with the new command's first word
                            strncpy(temp_for_prompt_name, trimmed_cmd, sizeof(temp_for_prompt_name)-1);
                            temp_for_prompt_name[sizeof(temp_for_prompt_name)-1] = '\0';
                            first_word = strtok(temp_for_prompt_name, " \t\n\r");
                            if (first_word) strncpy(command_name_for_prompt, first_word, MAX_COMMAND_LEN -1);
                        } else {
                            // Error already printed by get_kth_history_element if k was invalid
                            time_taken_for_prompt = time(NULL) - start_time;
                            continue;
                        }
                    }
                }
            }


            int argc = parse_arguments(trimmed_cmd, args, MAX_ARGS, &is_background_job);
            if (argc == 0) {
                time_taken_for_prompt = time(NULL) - start_time;
                continue;
            }

            // Built-in commands
            if (strcmp(args[0], "q") == 0 || strcmp(args[0], "quit") == 0 || strcmp(args[0], "exit") == 0) {
                printf("Goodbye!\n");
                free(semi_colon_commands);
                write_history_to_file(history_queue, home_dir);
                destroyQue(history_queue);
                return EXIT_SUCCESS;
            } else if (strcmp(args[0], "clear") == 0) {
                system("clear");
            } else if (strcmp(args[0], "warp") == 0) {
                if (argc < 2) {
                    char* result = warp("~", home_dir, prev_dir); // Warp to home
                    if (result && strlen(result) > 0) {
                        strncpy(prev_dir, result, sizeof(prev_dir) - 1);
                        prev_dir[sizeof(prev_dir) - 1] = '\0';
                    }
                    free(result);
                } else {
                    for (int j = 1; j < argc; j++) {
                        char* result = warp(args[j], home_dir, prev_dir);
                        if (result && strlen(result) > 0) {
                             strncpy(prev_dir, result, sizeof(prev_dir) - 1);
                             prev_dir[sizeof(prev_dir) - 1] = '\0';
                        }
                        free(result);
                    }
                }
            } else if (strcmp(args[0], "peek") == 0) {
                // Simplified peek flag handling. A full getopt solution is better.
                bool l_flag = false, a_flag = false;
                char* target_peek_dir = NULL;
                for(int k=1; k<argc; ++k) {
                    if(args[k][0] == '-') {
                        for(size_t char_idx = 1; char_idx < strlen(args[k]); ++char_idx) {
                            if(args[k][char_idx] == 'l') l_flag = true;
                            else if(args[k][char_idx] == 'a') a_flag = true;
                            else print_shell_error("peek: Invalid flag option.");
                        }
                    } else {
                        if(target_peek_dir == NULL) target_peek_dir = args[k];
                        else print_shell_error("peek: Multiple directories specified.");
                    }
                }
                peek_execute(target_peek_dir, home_dir, l_flag, a_flag);

            } else if (strcmp(args[0], "pastevents") == 0) {
                // "execute" is handled above. This is for "pastevents" or "pastevents purge"
                if (argc == 1) {
                    display_history(history_queue);
                } else if (argc == 2 && strcmp(args[1], "purge") == 0) {
                    purge_history(history_queue);
                    write_history_to_file(history_queue, home_dir); // Persist purge
                } else {
                    print_shell_error("pastevents: Invalid arguments. Usage: pastevents [purge]");
                }
            } else if (strcmp(args[0], "proclore") == 0) {
                if (argc == 1) proclore_execute(getpid(), home_dir);
                else if (argc == 2) proclore_execute(atoi(args[1]), home_dir);
                else print_shell_error("proclore: Too many arguments. Usage: proclore [pid]");
            } else if (strcmp(args[0], "seek") == 0) {
                // Simplified seek flag handling.
                bool d_flag = false, f_flag = false, e_flag = false;
                char* target_name = NULL;
                char* search_dir = "."; // Default to current directory
                int arg_idx = 1;
                while(arg_idx < argc && args[arg_idx][0] == '-') {
                    for(size_t char_idx = 1; char_idx < strlen(args[arg_idx]); ++char_idx) {
                        if(args[arg_idx][char_idx] == 'd') d_flag = true;
                        else if(args[arg_idx][char_idx] == 'f') f_flag = true;
                        else if(args[arg_idx][char_idx] == 'e') e_flag = true;
                        else print_shell_error("seek: Invalid flag.");
                    }
                    arg_idx++;
                }
                if(arg_idx < argc) target_name = args[arg_idx++];
                else { print_shell_error("seek: Target name not specified."); time_taken_for_prompt = time(NULL) - start_time; continue; }

                if(arg_idx < argc) search_dir = args[arg_idx];
                
                if (d_flag && f_flag) { print_shell_error("seek: Flags -d and -f are mutually exclusive."); }
                else seek_execute(target_name, search_dir, home_dir, prev_dir, d_flag, f_flag, e_flag);

            } else { // External command
                pid_t pid = fork();
                if (pid == -1) {
                    print_shell_perror("fork failed");
                } else if (pid == 0) { // Child process
                    setpgid(0, 0);
                    if (execvp(args[0], args) == -1) {
                        fprintf(stderr, _RED_ "Shell Error: Command '%s' not found or execvp error" _RESET_ ": %s\n", args[0], strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                } else { // Parent process
                    if (is_background_job) {
                        if (num_bg_processes < MAX_BG_PROCS) {
                            background_pids[num_bg_processes] = pid;
                            strncpy(background_process_names[num_bg_processes], args[0], MAX_PATH_LEN - 1);
                            background_process_names[num_bg_processes][MAX_PATH_LEN - 1] = '\0';
                            num_bg_processes++;
                            printf("Shell: Started background process [%d] %s (PID %d)\n", num_bg_processes, args[0], pid);
                        } else {
                            print_shell_error("Maximum background processes reached. Cannot run in background.");
                            int status; // Wait for it like a foreground job
                            waitpid(pid, &status, 0);
                        }
                    } else {
                        int status;
                        waitpid(pid, &status, 0);
                    }
                }
            }
            time_taken_for_prompt = time(NULL) - start_time;
        }

        if (add_original_to_history && strlen(original_input_for_history) > 0) {
            add_history_element(history_queue, original_input_for_history);
            write_history_to_file(history_queue, home_dir);
        }
        free(semi_colon_commands);
        semi_colon_commands = NULL;
    }

    write_history_to_file(history_queue, home_dir);
    destroyQue(history_queue);
    if (semi_colon_commands) free(semi_colon_commands); // Should be NULL if loop exited normally

    return EXIT_SUCCESS;
}