#include "utils/que.h"
#include "core/shell_state.h" // For constants like MAX_INPUT_LEN, HISTORY_SIZE, etc.
#include "utils/error.h"      // For print_shell_perror

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Que initQue() {
    Que Q = (Que)malloc(sizeof(struct que));
    if (!Q) {
        print_shell_perror("malloc for Que structure failed");
        exit(EXIT_FAILURE); // Critical failure
    }
    Q->capacity = HISTORY_SIZE;
    Q->arr = (Instruction*)malloc(sizeof(Instruction) * Q->capacity);
    if (!Q->arr) {
        print_shell_perror("malloc for Que array failed");
        free(Q);
        exit(EXIT_FAILURE); // Critical failure
    }
    for (int i = 0; i < Q->capacity; i++) {
        Q->arr[i] = (Instruction)malloc(sizeof(char) * MAX_INPUT_LEN);
        if (!Q->arr[i]) {
            print_shell_perror("malloc for Que instruction string failed");
            for (int j = 0; j < i; j++) free(Q->arr[j]);
            free(Q->arr);
            free(Q);
            exit(EXIT_FAILURE); // Critical failure
        }
        Q->arr[i][0] = '\0'; // Initialize to empty string
    }
    Q->latest = -1;
    Q->numElems = 0;
    return Q;
}

bool isHistoryEmpty(Que Q) {
    if (!Q) return true;
    return Q->numElems == 0;
}

void add_history_element(Que Q, Instruction e) {
    if (!Q || !e || strlen(e) == 0) return;

    // Avoid adding consecutive duplicates
    if (Q->numElems > 0 && strcmp(e, Q->arr[Q->latest]) == 0) {
        return;
    }

    Q->latest = (Q->latest + 1) % Q->capacity;
    strncpy(Q->arr[Q->latest], e, MAX_INPUT_LEN - 1);
    Q->arr[Q->latest][MAX_INPUT_LEN - 1] = '\0'; // Ensure null termination

    if (Q->numElems < Q->capacity) {
        Q->numElems++;
    }
}

Instruction get_kth_history_element(Que Q, int k) {
    if (!Q || k <= 0 || k > Q->numElems) {
        if (Q && Q->numElems > 0) { // Only print error if history is not empty
             fprintf(stderr, _RED_ "Shell Error: " _RESET_ "Invalid history index k=%d (history size is %d)\n", k, Q->numElems);
        } else if (Q && Q->numElems == 0) {
             fprintf(stderr, _RED_ "Shell Error: " _RESET_ "History is empty.\n");
        }
        return NULL;
    }
    // k=1 is latest, k=2 is second latest, etc.
    // (latest - (k-1) + capacity) % capacity
    int target_index = (Q->latest - (k - 1) + Q->capacity) % Q->capacity;

    Instruction ins_copy = (Instruction)malloc(sizeof(char) * MAX_INPUT_LEN);
    if (!ins_copy) {
        print_shell_perror("malloc for history element copy failed");
        return NULL;
    }
    strncpy(ins_copy, Q->arr[target_index], MAX_INPUT_LEN - 1);
    ins_copy[MAX_INPUT_LEN - 1] = '\0';
    return ins_copy; // Caller must free this
}

void purge_history(Que Q) {
    if (!Q) return;
    for (int i = 0; i < Q->capacity; i++) {
        Q->arr[i][0] = '\0';
    }
    Q->latest = -1;
    Q->numElems = 0;
    printf("Shell: In-memory history purged.\n");
}

void display_history(Que Q) {
    if (!Q || Q->numElems == 0) {
        printf("Shell: History is empty.\n");
        return;
    }
    // Iterate from oldest to newest
    // Oldest is at (latest - numElems + 1 + capacity) % capacity
    int current_idx = (Q->latest - Q->numElems + 1 + Q->capacity) % Q->capacity;
    for (int i = 0; i < Q->numElems; i++) {
        printf("%s\n", Q->arr[current_idx]);
        current_idx = (current_idx + 1) % Q->capacity;
    }
}

void read_history_from_file(Que Q, const char* homeDir) {
    if (!Q || !homeDir) return;
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/%s", homeDir, HISTORY_FILENAME);

    FILE* f = fopen(path, "r");
    if (!f) {
        // This is normal on first run or if file was deleted
        // perror("Could not open history file for reading"); 
        return;
    }

    char buff[MAX_INPUT_LEN];
    while (fgets(buff, sizeof(buff), f) != NULL) {
        size_t len = strlen(buff);
        if (len > 0 && buff[len - 1] == '\n') {
            buff[len - 1] = '\0';
        }
        if (strlen(buff) > 0) { // Don't add empty lines from file
            add_history_element(Q, buff); // add_history_element handles duplicates and capacity
        }
    }
    fclose(f);
}

void write_history_to_file(Que Q, const char* homeDir) {
    if (!Q || !homeDir) return;
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/%s", homeDir, HISTORY_FILENAME);

    FILE* f = fopen(path, "w");
    if (!f) {
        print_shell_perror("Could not open history file for writing");
        return;
    }

    if (Q->numElems == 0) {
        fclose(f);
        return;
    }

    int current_idx = (Q->latest - Q->numElems + 1 + Q->capacity) % Q->capacity;
    for (int i = 0; i < Q->numElems; i++) {
        fprintf(f, "%s\n", Q->arr[current_idx]);
        current_idx = (current_idx + 1) % Q->capacity;
    }
    fclose(f);
}

void destroyQue(Que Q) {
    if (!Q) return;
    for (int i = 0; i < Q->capacity; i++) {
        free(Q->arr[i]);
    }
    free(Q->arr);
    free(Q);
}

// src/que.c

// ... (existing function implementations)

int get_history_size(Que Q) {
    if (!Q) return 0;
    return Q->numElems;
}

Instruction get_kth_history_element_silent(Que Q, int k) {
    if (!Q || k <= 0 || k > Q->numElems) {
        return NULL; // Return NULL silently
    }
    // k=1 is latest, k=2 is second latest, etc.
    int target_index = (Q->latest - (k - 1) + Q->capacity) % Q->capacity;

    Instruction ins_copy = (Instruction)malloc(sizeof(char) * MAX_INPUT_LEN);
    if (!ins_copy) {
        print_shell_perror("malloc for history element copy failed");
        return NULL;
    }
    strncpy(ins_copy, Q->arr[target_index], MAX_INPUT_LEN - 1);
    ins_copy[MAX_INPUT_LEN - 1] = '\0';
    return ins_copy; // Caller must free this
}