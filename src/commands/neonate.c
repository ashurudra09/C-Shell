#include "commands/neonate.h"
#include "utils/error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>
#include <stdbool.h>

static struct termios orig_termios;

static void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static bool enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        print_shell_perror("neonate: tcgetattr failed");
        return false;
    }
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        print_shell_perror("neonate: tcsetattr failed");
        return false;
    }
    return true;
}

static int filter_pids(const struct dirent *entry) {
    for (int i = 0; entry->d_name[i]; i++) {
        if (!isdigit(entry->d_name[i])) return 0;
    }
    return 1;
}

// --- START OF THE FIX ---
/**
 * @brief A custom comparison function for scandir to sort PIDs numerically.
 *
 * This function is passed to scandir to ensure that directory names
 * representing PIDs are sorted as integers, not as strings.
 *
 * @param a The first directory entry.
 * @param b The second directory entry.
 * @return An integer less than, equal to, or greater than zero if the first
 *         argument is considered to be respectively less than, equal to, or
 *         greater than the second.
 */
static int numeric_sort(const struct dirent **a, const struct dirent **b) {
    int pid_a = atoi((*a)->d_name);
    int pid_b = atoi((*b)->d_name);
    return pid_a - pid_b;
}
// --- END OF THE FIX ---

static void print_latest_pid() {
    struct dirent **namelist;
    // CRITICAL FIX: Replace 'alphasort' with our 'numeric_sort' function.
    int n = scandir("/proc", &namelist, filter_pids, numeric_sort);
    if (n < 0) {
        print_shell_perror("neonate: scandir failed");
        return;
    }

    if (n > 0) {
        // Because the list is now sorted NUMERICALLY, the last entry
        // is guaranteed to be the highest PID.
        printf("%s\n", namelist[n - 1]->d_name);
    }

    for (int i = 0; i < n; i++) {
        free(namelist[i]);
    }
    free(namelist);
}

void neonate_execute(int time_arg) {
    if (!enable_raw_mode()) {
        return;
    }

    time_t last_print_time = 0;
    // Immediately print the first PID without waiting
    print_latest_pid();
    last_print_time = time(NULL);

    while (1) {
        char c = '\0';
        if (read(STDIN_FILENO, &c, 1) == 1 && c == 'x') {
            break;
        }

        time_t current_time = time(NULL);
        if (current_time - last_print_time >= time_arg) {
            print_latest_pid();
            last_print_time = current_time;
        }
        usleep(100000); // 100ms sleep to prevent high CPU usage
    }

    disable_raw_mode();
}