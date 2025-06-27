#ifndef PING_H_
#define PING_H_

#include <sys/types.h> // For pid_t

/**
 * @brief Executes the 'ping' command to send a signal to a process.
 *
 * @param pid The Process ID of the target process.
 * @param signal_number The signal number to send.
 */
void ping_execute(pid_t pid, int signal_number);

#endif // PING_H_