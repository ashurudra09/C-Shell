#ifndef FG_BG_H_
#define FG_BG_H_

#include "core/shell_state.h"
#include <sys/types.h>

/**
 * @brief Brings a background job to the foreground.
 * @param pid The PID of any process within the job to bring to the foreground.
 * @param state A pointer to the current shell state.
 */
void fg_execute(pid_t pid, ShellState* state);

/**
 * @brief Resumes a stopped background job, keeping it in the background.
 * @param pid The PID of any process within the stopped job to resume.
 * @param state A pointer to the current shell state.
 */
void bg_execute(pid_t pid, ShellState* state);

#endif // FG_BG_H_