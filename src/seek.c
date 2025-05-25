#include "shell_headers.h"

// Forward declaration for recursive helper
static int seek_recursive(const char* target_name, const char* current_dir_path,
                           const char* base_search_dir_path, const char* home_dir, char* prev_dir_shell,
                           bool search_dirs_only, bool search_files_only, bool execute_on_match,
                           int* match_count, bool* executed_action);


/**
 * @brief Helper function to prepare the target directory path for seek.
 * Expands '~'.
 * @param input_dir_arg The directory argument from the user.
 * @param home_dir The user's home directory.
 * @param output_path_buffer Buffer to store the resolved path.
 * @param buffer_size Size of the output_path_buffer.
 * @return True if path resolution was successful, false otherwise.
 */
static bool resolve_seek_search_path(const char* input_dir_arg, const char* home_dir,
                                     char* output_path_buffer, size_t buffer_size) {
    if (!input_dir_arg || strlen(input_dir_arg) == 0) { // Default to current directory
        if (getcwd(output_path_buffer, buffer_size) == NULL) {
            print_shell_perror("seek: getcwd for search path failed");
            return false;
        }
    } else {
        if (input_dir_arg[0] == '~') {
            if (strlen(input_dir_arg) == 1 || (input_dir_arg[1] == '/' || input_dir_arg[1] == '\0')) {
                snprintf(output_path_buffer, buffer_size, "%s%s", home_dir, input_dir_arg + 1);
            } else {
                snprintf(output_path_buffer, buffer_size, "%s/%s", home_dir, input_dir_arg + 1);
            }
        } else { // Absolute or relative path
            // If relative, we need to make it absolute for consistent base_search_dir_path
            if (input_dir_arg[0] != '/') {
                char temp_cwd[MAX_PATH_LEN];
                if (getcwd(temp_cwd, sizeof(temp_cwd)) == NULL) {
                    print_shell_perror("seek: getcwd for relative path resolution failed");
                    return false;
                }
                snprintf(output_path_buffer, buffer_size, "%s/%s", temp_cwd, input_dir_arg);
            } else {
                strncpy(output_path_buffer, input_dir_arg, buffer_size - 1);
                output_path_buffer[buffer_size - 1] = '\0';
            }
        }
    }
    // Normalize path (remove trailing slashes unless it's root)
    size_t len = strlen(output_path_buffer);
    while (len > 1 && output_path_buffer[len - 1] == '/') {
        output_path_buffer[len - 1] = '\0';
        len--;
    }
    return true;
}


void seek_execute(const char* target_name, const char* search_dir_arg,
                  const char* home_dir, char* prev_dir_shell, // prev_dir_shell is mutable for warp
                  bool search_dirs_only, bool search_files_only, bool execute_on_match) {

    if (!target_name || strlen(target_name) == 0) {
        print_shell_error("seek: Target name not specified.");
        return;
    }

    char resolved_search_dir[MAX_PATH_LEN];
    if (!resolve_seek_search_path(search_dir_arg, home_dir, resolved_search_dir, sizeof(resolved_search_dir))) {
        return;
    }
    
    // Check if resolved_search_dir is actually a directory
    struct stat dir_stat;
    if (stat(resolved_search_dir, &dir_stat) != 0 || !S_ISDIR(dir_stat.st_mode)) {
        fprintf(stderr, _RED_ "Shell Error: " _RESET_ "seek: Search path '%s' is not a valid directory.\n", resolved_search_dir);
        return;
    }


    int match_count = 0;
    bool executed_action = false; // To ensure -e action happens only once

    seek_recursive(target_name, resolved_search_dir, resolved_search_dir, home_dir, prev_dir_shell,
                   search_dirs_only, search_files_only, execute_on_match,
                   &match_count, &executed_action);

    if (match_count == 0) {
        printf("No match found.\n");
    }
}

static int seek_recursive(const char* target_name, const char* current_dir_path,
                           const char* base_search_dir_path, const char* home_dir, char* prev_dir_shell,
                           bool search_dirs_only, bool search_files_only, bool execute_on_match,
                           int* match_count, bool* executed_action) {
    DIR* dir_stream = opendir(current_dir_path);
    if (!dir_stream) {
        // Suppress error for non-readable directories to mimic find behavior
        // fprintf(stderr, _RED_ "Shell Error: " _RESET_ "seek: Cannot open directory '%s': %s\n",
        //         current_dir_path, strerror(errno));
        return 0; // Indicate 0 found in this path
    }

    struct dirent* entry;
    while ((entry = readdir(dir_stream)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char item_full_path[MAX_PATH_LEN * 2];
        snprintf(item_full_path, sizeof(item_full_path), "%s/%s", current_dir_path, entry->d_name);

        struct stat item_stat;
        if (lstat(item_full_path, &item_stat) == -1) {
            // fprintf(stderr, _RED_ "Shell Error: " _RESET_ "seek: lstat failed for '%s': %s\n",
            //         item_full_path, strerror(errno));
            continue; // Skip if cannot stat
        }

        bool is_dir = S_ISDIR(item_stat.st_mode);
        bool is_file = S_ISREG(item_stat.st_mode);
        bool name_matches = (strcmp(entry->d_name, target_name) == 0);
        // For case where target_name might have extension, e.g. "file.txt"
        // And entry->d_name is "file.txt"
        // The original code used strncmp(entry->d_name, target, strlen(target))
        // This means "file" would match "file.txt". Let's stick to exact match for simplicity.
        // If prefix match is desired: name_matches = (strncmp(entry->d_name, target_name, strlen(target_name)) == 0);

        // Construct relative path for display
        char relative_display_path[MAX_PATH_LEN * 2] = "./"; // Default prefix
        if (strcmp(current_dir_path, base_search_dir_path) == 0) { // Item is in the top search dir
             strncat(relative_display_path, entry->d_name, sizeof(relative_display_path) - strlen(relative_display_path) -1);
        } else { // Item is in a subdirectory
            // current_dir_path = /base/sub1/sub2, base_search_dir_path = /base
            // relative part is sub1/sub2/entry_name
            const char* relative_part_of_current = current_dir_path + strlen(base_search_dir_path) +1; // +1 for slash
            if (strlen(base_search_dir_path) == 1 && base_search_dir_path[0] == '/') { // base is root
                 relative_part_of_current = current_dir_path +1;
            } else if (strlen(current_dir_path) <= strlen(base_search_dir_path)) { // Should not happen if logic is correct
                 relative_part_of_current = ""; // Safety
            }
            snprintf(relative_display_path, sizeof(relative_display_path), "./%s/%s",
                     relative_part_of_current, entry->d_name);
        }


        if (name_matches) {
            if ((is_dir && !search_files_only) || (is_file && !search_dirs_only) || (!search_dirs_only && !search_files_only)) {
                (*match_count)++;
                if (is_dir) printf(_BLUE_ "%s\n" _RESET_, relative_display_path);
                else if (is_file) printf(_GREEN_ "%s\n" _RESET_, relative_display_path);
                else printf("%s\n", relative_display_path); // Other types

                if (execute_on_match && !(*executed_action)) {
                    if (is_dir && (search_dirs_only || !search_files_only)) {
                        char* old_prev = warp(item_full_path, home_dir, prev_dir_shell);
                        if (old_prev && strlen(old_prev) > 0) {
                            strncpy(prev_dir_shell, old_prev, MAX_PATH_LEN -1);
                            prev_dir_shell[MAX_PATH_LEN-1] = '\0';
                        }
                        free(old_prev);
                        *executed_action = true;
                    } else if (is_file && (search_files_only || !search_dirs_only)) {
                        FILE* f_content = fopen(item_full_path, "r");
                        if (f_content) {
                            int c;
                            while ((c = fgetc(f_content)) != EOF) {
                                putchar(c);
                            }
                            fclose(f_content);
                            // Add a newline if content didn't end with one, for cleaner output
                            // if (last_char_printed != '\n') putchar('\n');
                        } else {
                            fprintf(stderr, _RED_ "Shell Error: " _RESET_ "seek: Could not open file '%s' for reading: %s\n",
                                    item_full_path, strerror(errno));
                        }
                        *executed_action = true;
                    }
                }
            }
        }

        // Recurse if it's a directory
        if (is_dir) {
            seek_recursive(target_name, item_full_path, base_search_dir_path, home_dir, prev_dir_shell,
                           search_dirs_only, search_files_only, execute_on_match,
                           match_count, executed_action);
            if (execute_on_match && *executed_action) { // If action taken in recursion, stop further search
                closedir(dir_stream);
                return *match_count;
            }
        }
    }
    closedir(dir_stream);
    return *match_count;
}