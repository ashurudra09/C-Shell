#ifndef ACTIVITIES_H_
#define ACTIVITIES_H_

#include "core/shell_state.h"

/**
 * @brief Executes the 'activities' command.
 *
 * Scans the /proc directory to find and list all processes that were
 * spawned by the current shell session. It displays their PID, command name,
 * and current state (Running/Stopped).
 *
 * @param state A read-only pointer to the current shell state.
 */
void activities_execute(const ShellState* state);

#endif // ACTIVITIES_H_