#ifndef NEONATE_H_
#define NEONATE_H_

/**
 * @brief Executes the 'neonate' command.
 *
 * Periodically prints the PID of the most recently created process on the system
 * until the 'x' key is pressed.
 *
 * @param time_arg The interval in seconds between printing the PID.
 */
void neonate_execute(int time_arg);

#endif // NEONATE_H_