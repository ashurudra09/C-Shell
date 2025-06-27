#ifndef EXECUTOR_H_
#define EXECUTOR_H_

#include "core/shell_state.h"

/**
 * @brief Processes a raw line of input from the user.
 *
 * This is the main entry point after receiving input. It handles splitting by ';',
 * deals with 'pastevents execute', and then calls the parser and executor for each command.
 *
 * @param input_line The raw string from the user.
 * @param state The current state of the shell.
 */
void process_input_line(char* input_line, ShellState* state);

#endif // EXECUTOR_H_