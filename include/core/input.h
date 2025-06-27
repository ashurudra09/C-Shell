#ifndef INPUT_H_
#define INPUT_H_

#include "core/shell_state.h"

/**
 * @brief Reads a line of input with history navigation enabled.
 * @param buffer The buffer to store the input line.
 * @param size The size of the buffer.
 * @param state The current shell state (for history and prompt redrawing).
 * @return 0 on success (Enter pressed), -1 on EOF (Ctrl+D).
 */
int get_line_with_history(char* buffer, int size, const ShellState* state);

#endif // INPUT_H_