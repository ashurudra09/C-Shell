#include "core/input.h"
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// This function is almost identical to the one in your main.c,
// but it takes a ShellState pointer instead of multiple arguments.
int get_line_with_history(char* buffer, int size, const ShellState* state) {
    struct termios old_term, new_term;
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    int buffer_pos = 0;
    buffer[0] = '\0';
    int c;
    int history_index = 0;
    char* temp_command_storage = NULL;

    while (1) {
        c = getchar();

        if (c == EOF || c == 4) { // Ctrl+D
            if (buffer_pos == 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
                return -1;
            }
        } else if (c == '\n') {
            free(temp_command_storage);
            tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
            printf("\n");
            return 0;
        } else if (c == 127 || c == '\b') {
            if (buffer_pos > 0) {
                buffer_pos--;
                buffer[buffer_pos] = '\0';
                printf("\b \b");
                fflush(stdout);
            }
        } else if (c == '\x1b') {
            if (getchar() == '[') {
                switch (getchar()) {
                    case 'A': // Up
                        if (history_index < get_history_size(state->history_queue)) {
                            if (history_index == 0) {
                                free(temp_command_storage);
                                temp_command_storage = strdup(buffer);
                            }
                            history_index++;
                            char* hist_cmd = get_kth_history_element_silent(state->history_queue, history_index);
                            if (hist_cmd) {
                                strncpy(buffer, hist_cmd, size - 1);
                                buffer[size - 1] = '\0';
                                buffer_pos = strlen(buffer);
                                free(hist_cmd);
                            }
                        }
                        break;
                    case 'B': // Down
                        if (history_index > 0) {
                            history_index--;
                            if (history_index == 0) {
                                strncpy(buffer, temp_command_storage ? temp_command_storage : "", size - 1);
                                buffer[size-1] = '\0';
                            } else {
                                char* hist_cmd = get_kth_history_element_silent(state->history_queue, history_index);
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
                printf("\r");
                display_shell_prompt(state);
                printf("%s\033[K", buffer);
                fflush(stdout);
            }
        } else if (isprint(c)) {
            if (buffer_pos < size - 1) {
                buffer[buffer_pos++] = c;
                buffer[buffer_pos] = '\0';
                putchar(c);
                fflush(stdout);
            }
        }
    }
}