#include "shell_headers.h"

/**
 * @brief Changes the current working directory.
 *
 * Supports ".", "..", "-", "~", "~/path", and absolute/relative paths.
 *
 * @param dir_arg The target directory argument string. Can be NULL or empty for warp to home.
 * @param home_dir The user's home directory path.
 * @param prev_dir_global A string containing the path of the OLDPWD.
 * @return A dynamically allocated string containing the path of the directory *before*
 *         a successful warp. The caller must free this string. Returns an empty,
 *         dynamically allocated string on failure (e.g. chdir fails, OLDPWD not set for "warp -").
 *         The returned string will also be empty if strdup fails.
 */
char* warp(const char* dir_arg, const char* home_dir, const char* prev_dir_global) {
    char current_cwd_before_warp[MAX_PATH_LEN];
    if (getcwd(current_cwd_before_warp, sizeof(current_cwd_before_warp)) == NULL) {
        print_shell_perror("warp: getcwd (before warp) failed");
        char* empty_ret = strdup("");
        if (!empty_ret) print_shell_perror("warp: strdup failed for empty return");
        return empty_ret ? empty_ret : ""; // Should ideally not return literal ""
    }

    char target_dir_processed[MAX_PATH_LEN];

    if (!dir_arg || strlen(dir_arg) == 0 || strcmp(dir_arg, "~") == 0) { // warp, warp ~, warp ""
        strncpy(target_dir_processed, home_dir, sizeof(target_dir_processed) - 1);
    } else if (strcmp(dir_arg, "-") == 0) {
        if (strlen(prev_dir_global) == 0) {
            print_shell_error("warp: OLDPWD not set");
            char* empty_ret = strdup("");
            if (!empty_ret) print_shell_perror("warp: strdup failed for OLDPWD error");
            return empty_ret ? empty_ret : "";
        }
        strncpy(target_dir_processed, prev_dir_global, sizeof(target_dir_processed) - 1);
    } else if (dir_arg[0] == '~' && (dir_arg[1] == '/' || dir_arg[1] == '\0')) { // ~/path or ~
        snprintf(target_dir_processed, sizeof(target_dir_processed), "%s%s", home_dir, dir_arg + 1);
    } else { // Relative or absolute path
        strncpy(target_dir_processed, dir_arg, sizeof(target_dir_processed) - 1);
    }
    target_dir_processed[sizeof(target_dir_processed) - 1] = '\0';


    if (chdir(target_dir_processed) < 0) {
        fprintf(stderr, _RED_ "Shell Error: " _RESET_ "warp: Failed to change directory to '%s': %s\n",
                target_dir_processed, strerror(errno));
        char* empty_ret = strdup("");
        if (!empty_ret) print_shell_perror("warp: strdup failed for chdir error");
        return empty_ret ? empty_ret : "";
    }

    // Print the new current directory (as per original functionality)
    char new_cwd_after_warp[MAX_PATH_LEN];
    if (getcwd(new_cwd_after_warp, sizeof(new_cwd_after_warp)) != NULL) {
        printf("%s\n", new_cwd_after_warp);
    } else {
        print_shell_perror("warp: getcwd (after warp) failed, though chdir succeeded");
    }

    char* old_cwd_to_return = strdup(current_cwd_before_warp);
    if (!old_cwd_to_return) {
        print_shell_perror("warp: strdup failed for old_cwd_to_return");
        // Fallback to returning an empty string if strdup fails catastrophically
        char* empty_ret = strdup("");
        return empty_ret ? empty_ret : "";
    }
    return old_cwd_to_return;
}