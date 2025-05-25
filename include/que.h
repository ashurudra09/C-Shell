#ifndef QUE_H_
#define QUE_H_

#include <stdbool.h> // For bool

/**
 * @brief Represents a single command/instruction string in the history.
 */
typedef char* Instruction;

/**
 * @brief Structure for a circular queue to store command history.
 */
struct que {
    Instruction* arr;  ///< Array of strings (commands).
    int latest;        ///< Index of the most recently added element.
    int numElems;      ///< Current number of elements in the queue.
    int capacity;      ///< Maximum capacity of the queue (HISTORY_SIZE).
};

/**
 * @brief Pointer to a Que structure.
 */
typedef struct que* Que;

/**
 * @brief Initializes a new history queue.
 * @return Pointer to the initialized Que, or NULL on failure.
 */
Que initQue();

/**
 * @brief Checks if the history queue is empty.
 * @param Q The history queue.
 * @return True if empty, false otherwise.
 */
bool isHistoryEmpty(Que Q);

/**
 * @brief Adds an element (command string) to the history queue.
 * Avoids adding consecutive duplicates.
 * @param Q The history queue.
 * @param e The command string to add.
 */
void add_history_element(Que Q, Instruction e);

/**
 * @brief Retrieves the k-th element from the history (1-indexed from most recent).
 * @param Q The history queue.
 * @param k The 1-based index (1 is the most recent).
 * @return A new dynamically allocated string containing the command, or NULL if k is invalid or on error.
 *         The caller is responsible for freeing the returned string.
 */
Instruction get_kth_history_element(Que Q, int k);

/**
 * @brief Clears all elements from the history queue (in-memory).
 * @param Q The history queue.
 */
void purge_history(Que Q);

/**
 * @brief Displays all commands currently in the history queue.
 * @param Q The history queue.
 */
void display_history(Que Q);

/**
 * @brief Reads command history from the history file into the queue.
 * @param Q The history queue.
 * @param homeDir The home directory path (to locate the history file).
 */
void read_history_from_file(Que Q, const char* homeDir);

/**
 * @brief Writes the current command history from the queue to the history file.
 * @param Q The history queue.
 * @param homeDir The home directory path (to locate the history file).
 */
void write_history_to_file(Que Q, const char* homeDir);

/**
 * @brief Frees all memory associated with the history queue.
 * @param Q The history queue.
 */
void destroyQue(Que Q);

#endif // QUE_H_