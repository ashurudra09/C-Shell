#include "core/parser.h"
#include "utils/error.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// This was previously in main.c
static bool parse_simple_command(char* command_str, SimpleCommand* cmd) {
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_mode = false;
    for (int i = 0; i < MAX_ARGS; i++) {
        cmd->args[i] = NULL;
    }

    char* saveptr;
    char temp_str[MAX_COMMAND_LEN];
    strncpy(temp_str, command_str, sizeof(temp_str) - 1);
    temp_str[sizeof(temp_str) - 1] = '\0';

    int argc = 0;
    char* token = strtok_r(temp_str, " \t\n\r", &saveptr);
    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            if (cmd->input_file) { print_shell_error("Syntax error: Ambiguous input redirect."); return false; }
            token = strtok_r(NULL, " \t\n\r", &saveptr);
            if (!token || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0 || strcmp(token, "<") == 0) {
                print_shell_error("Syntax error: Missing name for input redirect."); return false;
            }
            cmd->input_file = strdup(token);
        } else if (strcmp(token, ">") == 0) {
            if (cmd->output_file) { print_shell_error("Syntax error: Ambiguous output redirect."); return false; }
            token = strtok_r(NULL, " \t\n\r", &saveptr);
            if (!token || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0 || strcmp(token, "<") == 0) {
                print_shell_error("Syntax error: Missing name for output redirect."); return false;
            }
            cmd->output_file = strdup(token);
            cmd->append_mode = false;
        } else if (strcmp(token, ">>") == 0) {
            if (cmd->output_file) { print_shell_error("Syntax error: Ambiguous output redirect."); return false; }
            token = strtok_r(NULL, " \t\n\r", &saveptr);
            if (!token || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0 || strcmp(token, "<") == 0) {
                print_shell_error("Syntax error: Missing name for output redirect."); return false;
            }
            cmd->output_file = strdup(token);
            cmd->append_mode = true;
        } else {
            if (argc < MAX_ARGS - 1) {
                cmd->args[argc++] = strdup(token);
            }
        }
        token = strtok_r(NULL, " \t\n\r", &saveptr);
    }
    cmd->args[argc] = NULL;
    return true;
}

int parse_pipeline(char* command_str, SimpleCommand commands[], int max_commands, bool* is_background) {
    *is_background = false;
    char* ampersand = strrchr(command_str, '&');
    if (ampersand != NULL && (*(ampersand - 1) == ' ' || *(ampersand - 1) == '\t') && *(ampersand + 1) == '\0') {
        *is_background = true;
        *ampersand = '\0'; // Remove the ampersand from the string
    }

    int num_commands = 0;
    char* pipe_saveptr;
    char* pipe_segment = strtok_r(command_str, "|", &pipe_saveptr);

    while(pipe_segment != NULL && num_commands < max_commands) {
        while (isspace((unsigned char)*pipe_segment)) pipe_segment++;
        char* pipe_end = pipe_segment + strlen(pipe_segment) - 1;
        while (pipe_end > pipe_segment && isspace((unsigned char)*pipe_end)) pipe_end--;
        *(pipe_end + 1) = '\0';

        if (strlen(pipe_segment) == 0) {
            print_shell_error("Syntax error: Unexpected null command in pipeline.");
            free_simple_commands(commands, num_commands);
            return -1;
        }

        if (!parse_simple_command(pipe_segment, &commands[num_commands])) {
            free_simple_commands(commands, num_commands);
            return -1;
        }
        num_commands++;
        pipe_segment = strtok_r(NULL, "|", &pipe_saveptr);
    }

    return num_commands;
}

void free_simple_commands(SimpleCommand commands[], int num_commands) {
    for (int i = 0; i < num_commands; ++i) {
        for (int k = 0; commands[i].args[k] != NULL; ++k) {
            free(commands[i].args[k]);
        }
        free(commands[i].input_file);
        free(commands[i].output_file);
    }
}