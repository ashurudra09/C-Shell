#include "core/shell_state.h"
#include "core/input.h"
#include "core/executor.h"
#include "utils/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    ShellState state;
    if (!shell_state_init(&state)) {
        return EXIT_FAILURE;
    }

    char input_line[MAX_INPUT_LEN];

    while (state.is_running) {
        display_shell_prompt(&state);

        // Reset per-command prompt info
        state.last_command_name[0] = '\0';
        state.time_taken_for_prompt = -1;

        if (get_line_with_history(input_line, sizeof(input_line), &state) == -1) {
            // Ctrl+D on an empty line
            state.is_running = false;
            printf("Goodbye!\n");
            continue;
        }

        if (strlen(input_line) == 0) {
            continue;
        }

        char mutable_input_line[MAX_INPUT_LEN];
        strncpy(mutable_input_line, input_line, sizeof(mutable_input_line));
        mutable_input_line[sizeof(mutable_input_line) - 1] = '\0';

        // The executor now handles all processing for the input line
        process_input_line(mutable_input_line, &state);
    }

    shell_state_destroy(&state);
    return EXIT_SUCCESS;
}