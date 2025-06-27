#include "shell_headers.h"
#include <termios.h> // Required for terminal manipulation

// Forward declaration for the prompt function, needed inside get_line_with_history
void display_shell_prompt(const char* home_dir, const char* last_command_name, long time_taken);


/**
 * @brief Reads a line of input with history navigation enabled.
 * Replaces fgets for interactive input.
 * @param buffer The buffer to store the input line.
 * @param size The size of the buffer.
 * @param history The command history queue.
 * @param home_dir The home directory for displaying the prompt correctly.
 * @return 0 on success (Enter pressed), -1 on EOF (Ctrl+D).
 */
int get_line_with_history(char* buffer, int size, Que history, const char* home_dir) {
    // 1. Set terminal to non-canonical mode
    struct termios old_term, new_term;
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echoing
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    int buffer_pos = 0;
    buffer[0] = '\0';
    int c;

    int history_index = 0; // 0 = new command, 1 = latest, etc.
    char* temp_command_storage = NULL; // To store user's partial command

    while (1) {
        c = getchar();

        if (c == EOF || c == 4) { // Ctrl+D
            if (buffer_pos == 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &old_term); // Restore terminal
                return -1; // Signal EOF
            }
        } else if (c == '\n') { // Enter key
            free(temp_command_storage);
            tcsetattr(STDIN_FILENO, TCSANOW, &old_term); // Restore terminal
            printf("\n");
            return 0;
        } else if (c == 127 || c == '\b') { // Backspace
            if (buffer_pos > 0) {
                buffer_pos--;
                buffer[buffer_pos] = '\0';
                printf("\b \b"); // Move cursor back, write space, move back again
                fflush(stdout);
            }
        } else if (c == '\x1b') { // Escape sequence (arrow keys)
            // Read the next two characters
            if (getchar() == '[') {
                switch (getchar()) {
                    case 'A': // Up arrow
                        if (history_index < get_history_size(history)) {
                            if (history_index == 0) {
                                // Save the current line buffer before overwriting
                                free(temp_command_storage);
                                temp_command_storage = strdup(buffer);
                            }
                            history_index++;
                            char* hist_cmd = get_kth_history_element_silent(history, history_index);
                            if (hist_cmd) {
                                strncpy(buffer, hist_cmd, size - 1);
                                buffer[size - 1] = '\0';
                                buffer_pos = strlen(buffer);
                                free(hist_cmd);
                            }
                        }
                        break;
                    case 'B': // Down arrow
                        if (history_index > 0) {
                            history_index--;
                            if (history_index == 0) {
                                // Restore the original command
                                strncpy(buffer, temp_command_storage ? temp_command_storage : "", size - 1);
                                buffer[size-1] = '\0';
                            } else {
                                char* hist_cmd = get_kth_history_element_silent(history, history_index);
                                if (hist_cmd) {
                                    strncpy(buffer, hist_cmd, size - 1);
                                    buffer[size - 1] = '\0';
                                    free(hist_cmd);
                                }
                            }
                            buffer_pos = strlen(buffer);
                        }
                        break;
                }
                // Redraw the line
                printf("\r"); // Carriage return to start of line
                display_shell_prompt(home_dir, "", -1); // Redraw prompt
                printf("%s\033[K", buffer); // Print buffer and clear rest of line
                fflush(stdout);
            }
        } else if (isprint(c)) { // Printable character
            if (buffer_pos < size - 1) {
                buffer[buffer_pos++] = c;
                buffer[buffer_pos] = '\0';
                putchar(c);
                fflush(stdout);
            }
        }
    }
}

/**
 * @brief Parses a single command segment, handling I/O redirection and syntax errors.
 * @param command_str The string for a single command.
 * @param cmd Pointer to a SimpleCommand struct to populate.
 * @return true on successful parse, false on syntax error.
 */
bool parse_simple_command(char* command_str, SimpleCommand* cmd) {
    // Initialize the struct
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_mode = false;
    for (int i = 0; i < MAX_ARGS; i++) {
        cmd->args[i] = NULL;
    }

    char* token;
    int argc = 0;
    
    // strtok_r requires a save pointer
    char* saveptr; 

    char temp_str[MAX_COMMAND_LEN];
    strncpy(temp_str, command_str, sizeof(temp_str) - 1);
    temp_str[sizeof(temp_str) - 1] = '\0';

    token = strtok_r(temp_str, " \t\n\r", &saveptr); // Use strtok_r
    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            if (cmd->input_file) {
                print_shell_error("Syntax error: Ambiguous input redirect.");
                return false;
            }
            token = strtok_r(NULL, " \t\n\r", &saveptr); // Use strtok_r
            if (!token || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0 || strcmp(token, "<") == 0) {
                print_shell_error("Syntax error: Missing name for input redirect.");
                return false;
            }
            cmd->input_file = strdup(token);
        } else if (strcmp(token, ">") == 0) {
            if (cmd->output_file) {
                print_shell_error("Syntax error: Ambiguous output redirect.");
                return false;
            }
            token = strtok_r(NULL, " \t\n\r", &saveptr); // Use strtok_r
            if (!token || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0 || strcmp(token, "<") == 0) {
                print_shell_error("Syntax error: Missing name for output redirect.");
                return false;
            }
            cmd->output_file = strdup(token);
            cmd->append_mode = false;
        } else if (strcmp(token, ">>") == 0) {
            if (cmd->output_file) {
                print_shell_error("Syntax error: Ambiguous output redirect.");
                return false;
            }
            token = strtok_r(NULL, " \t\n\r", &saveptr); // Use strtok_r
            if (!token || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0 || strcmp(token, "<") == 0) {
                print_shell_error("Syntax error: Missing name for output redirect.");
                return false;
            }
            cmd->output_file = strdup(token);
            cmd->append_mode = true;
        } else {
            if (argc < MAX_ARGS - 1) {
                cmd->args[argc++] = strdup(token);
            }
        }
        token = strtok_r(NULL, " \t\n\r", &saveptr); // Use strtok_r
    }
    cmd->args[argc] = NULL;
    return true; // Success
}

/**
 * @brief Executes a pipeline of one or more commands.
 * @param commands Array of SimpleCommand structs.
 * @param num_commands The number of commands in the pipeline.
 * @param is_background Whether the entire pipeline should run in the background.
 * @param home_dir The shell's home directory.
 * @param prev_dir The shell's previous directory.
 * @param background_pids Array for tracking background process PIDs.
 * @param bg_process_names Array for tracking background process names.
 * @param num_bg_processes Pointer to the number of background processes.
 */
void execute_pipeline(SimpleCommand commands[], int num_commands, bool is_background, int background_pids[], char bg_process_names[][MAX_PATH_LEN], int* num_bg_processes) {
    int input_fd = STDIN_FILENO;
    int pipe_fds[2];
    pid_t pids[num_commands];

    for (int i = 0; i < num_commands; i++) {
        // Create a pipe for all but the last command
        if (i < num_commands - 1) {
            if (pipe(pipe_fds) < 0) {
                print_shell_perror("pipe failed");
                return;
            }
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            print_shell_perror("fork failed");
            return;
        }

        if (pids[i] == 0) { // --- Child Process ---
            // Handle input redirection
            if (i > 0) { // Not the first command
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            if (commands[i].input_file) {
                int in_fd = open(commands[i].input_file, O_RDONLY);
                if (in_fd < 0) {
                    print_shell_perror(commands[i].input_file);
                    exit(EXIT_FAILURE);
                }
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            // Handle output redirection
            if (i < num_commands - 1) { // Not the last command
                dup2(pipe_fds[1], STDOUT_FILENO);
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }
            if (commands[i].output_file) {
                int flags = O_WRONLY | O_CREAT;
                flags |= commands[i].append_mode ? O_APPEND : O_TRUNC;
                int out_fd = open(commands[i].output_file, flags, 0644);
                if (out_fd < 0) {
                    print_shell_perror(commands[i].output_file);
                    exit(EXIT_FAILURE);
                }
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }

            // Check for built-in commands (only if it's a single command in the pipeline)
            // This is a simplification; handling built-ins in pipes is more complex.
            if (num_commands == 1 && strcmp(commands[i].args[0], "warp") == 0) {
                // 'warp' must run in the parent. We exit here, parent will handle it.
                exit(EXIT_SUCCESS); 
            }
            // Add other built-ins here if they need special handling.
            
            if (execvp(commands[i].args[0], commands[i].args) == -1) {
                fprintf(stderr, _RED_ "Shell Error: Command '%s' not found or execvp error" _RESET_ ": %s\n", commands[i].args[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        // --- Parent Process ---
        if (input_fd != STDIN_FILENO) {
            close(input_fd);
        }
        if (i < num_commands - 1) {
            close(pipe_fds[1]);
            input_fd = pipe_fds[0];
        }
    }

    // --- Parent waits for all children in the pipeline ---
    if (!is_background) {
        for (int i = 0; i < num_commands; i++) {
            int status;
            waitpid(pids[i], &status, 0);
        }
    } else {
        if (*num_bg_processes < MAX_BG_PROCS) {
            background_pids[*num_bg_processes] = pids[num_commands - 1]; // Track the last process in the pipe
            strncpy(bg_process_names[*num_bg_processes], commands[num_commands-1].args[0], MAX_PATH_LEN - 1);
            bg_process_names[*num_bg_processes][MAX_PATH_LEN - 1] = '\0';
            (*num_bg_processes)++;
            printf("Shell: Started background pipeline [%d] (last PID %d)\n", *num_bg_processes, pids[num_commands-1]);
        } else {
            print_shell_error("Maximum background processes reached.");
            for (int i = 0; i < num_commands; i++) waitpid(pids[i], NULL, 0);
        }
    }
}

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

        if (get_line_with_history(input_line, sizeof(input_line), history_queue, home_dir) == -1) {
            // -1 signifies Ctrl+D on an empty line
            printf("Goodbye!\n");
            break;
        }

        strncpy(original_input_for_history, input_line, sizeof(original_input_for_history) - 1);
        original_input_for_history[sizeof(original_input_for_history) - 1] = '\0';

        char mutable_input_line[MAX_INPUT_LEN];
        strncpy(mutable_input_line, input_line, sizeof(mutable_input_line) -1);
        mutable_input_line[sizeof(mutable_input_line)-1] = '\0';
        
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

            char* trimmed_cmd_ptr = command_to_process_storage;
            while (isspace((unsigned char)*trimmed_cmd_ptr)) trimmed_cmd_ptr++;
            char* end = trimmed_cmd_ptr + strlen(trimmed_cmd_ptr) - 1;
            while (end > trimmed_cmd_ptr && isspace((unsigned char)*end)) end--;
            *(end + 1) = '\0';

            if (strlen(trimmed_cmd_ptr) == 0) continue;

            // --- FIXED: 'pastevents execute' logic ---
            if (strncmp(trimmed_cmd_ptr, "pastevents execute", 18) == 0) {
                char temp_pastevents_cmd[MAX_COMMAND_LEN];
                strncpy(temp_pastevents_cmd, trimmed_cmd_ptr, sizeof(temp_pastevents_cmd)-1);
                char* pe_saveptr;
                strtok_r(temp_pastevents_cmd, " \t\n\r", &pe_saveptr); // "pastevents"
                strtok_r(NULL, " \t\n\r", &pe_saveptr); // "execute"
                char* pe_token = strtok_r(NULL, " \t\n\r", &pe_saveptr); // The number k
                if (!pe_token) { print_shell_error("pastevents execute: Number not given"); continue; }
                int k = atoi(pe_token);
                if (k <= 0) { print_shell_error("pastevents execute: Invalid number for k"); continue; }
                char* command_from_history = get_kth_history_element(history_queue, k);
                if (command_from_history) {
                    // BUG FIX: Copy to the start of the buffer, not to the trimmed pointer
                    strncpy(command_to_process_storage, command_from_history, MAX_COMMAND_LEN-1);
                    // We must now re-trim the new command in the buffer
                    trimmed_cmd_ptr = command_to_process_storage;
                    while (isspace((unsigned char)*trimmed_cmd_ptr)) trimmed_cmd_ptr++;
                    end = trimmed_cmd_ptr + strlen(trimmed_cmd_ptr) - 1;
                    while (end > trimmed_cmd_ptr && isspace((unsigned char)*end)) end--;
                    *(end + 1) = '\0';

                    add_history_element(history_queue, command_from_history);
                    write_history_to_file(history_queue, home_dir);
                    add_original_to_history = false;
                    free(command_from_history);
                } else { continue; }
            }

            long start_time = time(NULL);
            
            bool is_background_job = false;
            char* ampersand = strrchr(trimmed_cmd_ptr, '&');
            if (ampersand != NULL && (*(ampersand - 1) == ' ' || *(ampersand - 1) == '\t') && *(ampersand + 1) == '\0') {
                is_background_job = true;
                *ampersand = '\0';
            }

            SimpleCommand commands[MAX_ARGS];
            int num_commands = 0;
            bool parsing_ok = true;
            
            // --- FIXED: Use strtok_r for the outer pipe parsing loop ---
            char* pipe_saveptr;
            char* pipe_segment = strtok_r(trimmed_cmd_ptr, "|", &pipe_saveptr);

            while(pipe_segment != NULL && num_commands < MAX_ARGS) {
                while (isspace((unsigned char)*pipe_segment)) pipe_segment++;
                char* pipe_end = pipe_segment + strlen(pipe_segment) - 1;
                while (pipe_end > pipe_segment && isspace((unsigned char)*pipe_end)) pipe_end--;
                *(pipe_end + 1) = '\0';

                if (strlen(pipe_segment) == 0) {
                    print_shell_error("Syntax error: Unexpected null command in pipeline.");
                    parsing_ok = false;
                    break;
                }

                if (!parse_simple_command(pipe_segment, &commands[num_commands])) {
                    parsing_ok = false;
                    break;
                }
                num_commands++;
                
                // Continue with the same save pointer for pipes
                pipe_segment = strtok_r(NULL, "|", &pipe_saveptr);
            }

            if (!parsing_ok) {
                for(int j=0; j<num_commands; ++j) {
                    for(int k=0; commands[j].args[k] != NULL; ++k) free(commands[j].args[k]);
                    free(commands[j].input_file);
                    free(commands[j].output_file);
                }
                continue;
            }

            if (num_commands == 0 || commands[0].args[0] == NULL) {
                continue;
            }

            // For prompt display
            strncpy(command_name_for_prompt, commands[0].args[0], MAX_COMMAND_LEN - 1);
            command_name_for_prompt[MAX_COMMAND_LEN - 1] = '\0';

            // --- BUILT-IN COMMAND HANDLING ---
            // Built-ins are handled only if they are the ONLY command in the line (no pipes).
            if (num_commands == 1) {
                char* cmd_name = commands[0].args[0];
                
                // Calculate argc for the specific command
                int argc = 0;
                while(commands[0].args[argc] != NULL) argc++;

                if (strcmp(cmd_name, "q") == 0 || strcmp(cmd_name, "quit") == 0 || strcmp(cmd_name, "exit") == 0) {
                    printf("Goodbye!\n");
                    for(int j=0; j<num_commands; ++j) {
                        for(int k=0; commands[j].args[k] != NULL; ++k) free(commands[j].args[k]);
                        free(commands[j].input_file);
                        free(commands[j].output_file);
                    }
                    free(semi_colon_commands);
                    write_history_to_file(history_queue, home_dir);
                    destroyQue(history_queue);
                    return EXIT_SUCCESS;

                } else if (strcmp(cmd_name, "warp") == 0) {
                    if (argc < 2) {
                        char* result = warp("~", home_dir, prev_dir);
                        if (result && strlen(result) > 0) {
                            strncpy(prev_dir, result, sizeof(prev_dir) - 1);
                            prev_dir[sizeof(prev_dir) - 1] = '\0';
                        }
                        free(result);
                    } else {
                        for (int j = 1; j < argc; j++) {
                            char* result = warp(commands[0].args[j], home_dir, prev_dir);
                            if (result && strlen(result) > 0) {
                                strncpy(prev_dir, result, sizeof(prev_dir) - 1);
                                prev_dir[sizeof(prev_dir) - 1] = '\0';
                            }
                            free(result);
                        }
                    }
                } else if (strcmp(cmd_name, "peek") == 0) {
                    bool l_flag = false, a_flag = false;
                    char* target_peek_dir = NULL;
                    for(int k=1; k<argc; ++k) {
                        if(commands[0].args[k][0] == '-') {
                            for(size_t char_idx = 1; char_idx < strlen(commands[0].args[k]); ++char_idx) {
                                if(commands[0].args[k][char_idx] == 'l') l_flag = true;
                                else if(commands[0].args[k][char_idx] == 'a') a_flag = true;
                                else print_shell_error("peek: Invalid flag option.");
                            }
                        } else {
                            if(target_peek_dir == NULL) target_peek_dir = commands[0].args[k];
                            else print_shell_error("peek: Multiple directories specified.");
                        }
                    }
                    peek_execute(target_peek_dir, home_dir, l_flag, a_flag);

                } else if (strcmp(cmd_name, "pastevents") == 0) {
                    // 'execute' is handled before parsing. This only handles display and purge.
                    if (argc == 1) {
                        display_history(history_queue);
                    } else if (argc == 2 && strcmp(commands[0].args[1], "purge") == 0) {
                        purge_history(history_queue);
                        write_history_to_file(history_queue, home_dir);
                    } else {
                        print_shell_error("pastevents: Invalid arguments. Usage: pastevents [purge]");
                    }
                } else if (strcmp(cmd_name, "proclore") == 0) {
                    if (argc == 1) proclore_execute(getpid(), home_dir);
                    else if (argc == 2) proclore_execute(atoi(commands[0].args[1]), home_dir);
                    else print_shell_error("proclore: Too many arguments. Usage: proclore [pid]");

                } else if (strcmp(cmd_name, "seek") == 0) {
                    bool d_flag = false, f_flag = false, e_flag = false;
                    char* target_name = NULL;
                    char* search_dir = ".";
                    int arg_idx = 1;
                    while(arg_idx < argc && commands[0].args[arg_idx][0] == '-') {
                        for(size_t char_idx = 1; char_idx < strlen(commands[0].args[arg_idx]); ++char_idx) {
                            if(commands[0].args[arg_idx][char_idx] == 'd') d_flag = true;
                            else if(commands[0].args[arg_idx][char_idx] == 'f') f_flag = true;
                            else if(commands[0].args[arg_idx][char_idx] == 'e') e_flag = true;
                            else print_shell_error("seek: Invalid flag.");
                        }
                        arg_idx++;
                    }
                    if(arg_idx < argc) target_name = commands[0].args[arg_idx++];
                    else { print_shell_error("seek: Target name not specified."); time_taken_for_prompt = time(NULL) - start_time; continue; }

                    if(arg_idx < argc) search_dir = commands[0].args[arg_idx];
                    
                    if (d_flag && f_flag) { print_shell_error("seek: Flags -d and -f are mutually exclusive."); }
                    else seek_execute(target_name, search_dir, home_dir, prev_dir, d_flag, f_flag, e_flag);

                } else {
                    // Not a built-in, so execute as an external command/pipeline
                    execute_pipeline(commands, num_commands, is_background_job, background_pids, background_process_names, &num_bg_processes);
                }
            } else {
                // This is a pipeline with more than one command
                execute_pipeline(commands, num_commands, is_background_job, background_pids, background_process_names, &num_bg_processes);
            }
            
            time_taken_for_prompt = time(NULL) - start_time;

            // Cleanup dynamically allocated strings from parsing
            for(int j=0; j<num_commands; ++j) {
                for(int k=0; commands[j].args[k] != NULL; ++k) free(commands[j].args[k]);
                free(commands[j].input_file);
                free(commands[j].output_file);
            }
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