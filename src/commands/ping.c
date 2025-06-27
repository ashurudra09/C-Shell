#include "commands/ping.h"
#include "utils/error.h"

#include <signal.h> // For kill()
#include <errno.h>

// Your original 'ping' function, renamed and adapted for consistency.
void ping_execute(pid_t pid, int signal_number) {
    // Preserve the original signal number wrapping logic.
    signal_number %= 32;
    if (signal_number < 0) {
        signal_number += 32;
    }

    // Use kill() to send the signal.
    // On failure, use the project's standard error reporting function.
    if (kill(pid, signal_number) == -1) {
        print_shell_perror("ping"); // Provides a more descriptive error, e.g., "ping: No such process"
    }
}