#include "core/signals.h"
#include "core/shell_state.h"

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

// A global pointer to the shell state, necessary for signal handlers
// to access information about the current foreground process.
extern ShellState* g_shell_state;

/**
 * @brief Handler for SIGINT (Ctrl+C).
 */
static void sigint_handler(int signo) {
    (void)signo; // Unused parameter

    if (g_shell_state != NULL && g_shell_state->foreground_pgid > 0) {
        // If a foreground process is running, send SIGINT to its entire process group.
        kill(-g_shell_state->foreground_pgid, SIGINT);
    }
    // If no foreground process is running, the signal does nothing to the shell itself,
    // but we print a newline to keep the terminal clean and redraw the prompt.
    printf("\n");
    display_shell_prompt(g_shell_state);
}

/**
 * @brief Handler for SIGTSTP (Ctrl+Z).
 */
static void sigtstp_handler(int signo) {
    (void)signo; // Unused parameter

    if (g_shell_state != NULL && g_shell_state->foreground_pgid > 0) {
        // If a foreground process is running, send SIGTSTP to its entire process group.
        kill(-g_shell_state->foreground_pgid, SIGTSTP);
    }
    // If no foreground process is running, this signal is ignored.
}

void setup_signal_handlers() {
    struct sigaction sa_int, sa_tstp;

    // Setup SIGINT handler
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART; // Restart syscalls if interrupted by this signal
    sigaction(SIGINT, &sa_int, NULL);

    // Setup SIGTSTP handler
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);

    // Ignore signals that a shell should typically ignore for job control
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}