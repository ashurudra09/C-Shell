#include "utils/error.h"

void print_shell_error(const char* message) {
    fprintf(stderr, _RED_ "Shell Error: " _RESET_ "%s\n", message);
}

void print_shell_perror(const char* message) {
    fprintf(stderr, _RED_ "Shell Error: " _RESET_ "%s: %s\n", message, strerror(errno));
}