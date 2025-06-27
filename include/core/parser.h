#ifndef PARSER_H_
#define PARSER_H_

#include "core/shell_state.h"

/**
 * @brief Parses a pipeline string into an array of SimpleCommand structs.
 *
 * This function takes a single command string (which may contain pipes and
 * redirections) and populates an array of SimpleCommand structs.
 *
 * @param command_str The string for the pipeline (e.g., "cat < in.txt | grep 'a' > out.txt").
 * @param commands Array of SimpleCommand structs to be populated.
 * @param max_commands The maximum size of the commands array.
 * @param is_background Pointer to a boolean that will be set to true if the command ends with '&'.
 * @return The number of simple commands parsed from the pipeline, or -1 on syntax error.
 */
int parse_pipeline(char* command_str, SimpleCommand commands[], int max_commands, bool* is_background);

/**
 * @brief Frees the memory allocated within an array of SimpleCommand structs.
 * @param commands Array of SimpleCommand structs.
 * @param num_commands The number of valid commands in the array.
 */
void free_simple_commands(SimpleCommand commands[], int num_commands);

#endif // PARSER_H_