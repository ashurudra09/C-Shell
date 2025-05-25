#ifndef PROCLORE_H_
#define PROCLORE_H_

/**
 * @brief Displays information about a process.
 *
 * Information includes PID, process state, process group, virtual memory size,
 * and executable path (relative to home_dir if applicable).
 *
 * @param pid The Process ID to get information for.
 * @param home_dir The user's home directory path (for path relativization).
 */
void proclore_execute(int pid, const char* home_dir);

#endif // PROCLORE_H_