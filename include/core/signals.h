#ifndef SIGNALS_H_
#define SIGNALS_H_

/**
 * @brief Sets up the main signal handlers for the shell.
 *
 * This function should be called once at shell startup. It configures handlers
 * for SIGINT (Ctrl+C) and SIGTSTP (Ctrl+Z) and tells the shell to ignore
 * signals related to terminal control, which is crucial for job management.
 */
void setup_signal_handlers();

#endif // SIGNALS_H_