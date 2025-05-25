#include "shell_headers.h"

/**
 * @brief Helper function to prepare the target directory path for peek.
 * Expands '~', handles NULL dir_arg for current directory.
 * @param input_dir_arg The directory argument from the user.
 * @param home_dir The user's home directory.
 * @param output_path_buffer Buffer to store the resolved path.
 * @param buffer_size Size of the output_path_buffer.
 * @return True if path resolution was successful, false otherwise.
 */
static bool resolve_peek_path(const char* input_dir_arg, const char* home_dir,
                              char* output_path_buffer, size_t buffer_size) {
    if (!input_dir_arg || strlen(input_dir_arg) == 0) { // Peek current directory
        if (getcwd(output_path_buffer, buffer_size) == NULL) {
            print_shell_perror("peek: getcwd failed");
            return false;
        }
    } else {
        if (input_dir_arg[0] == '~') {
            if (strlen(input_dir_arg) == 1 || (input_dir_arg[1] == '/' || input_dir_arg[1] == '\0')) { // ~ or ~/path
                snprintf(output_path_buffer, buffer_size, "%s%s", home_dir, input_dir_arg + 1);
            } else { // ~somethingelse (treat as relative to home for simplicity, e.g. ~/somethingelse)
                snprintf(output_path_buffer, buffer_size, "%s/%s", home_dir, input_dir_arg + 1);
            }
        } else { // Absolute or relative path
            strncpy(output_path_buffer, input_dir_arg, buffer_size - 1);
            output_path_buffer[buffer_size - 1] = '\0';
        }
    }
    return true;
}

void print_file_permissions(mode_t perms) {
    printf((S_ISDIR(perms)) ? _CYAN_ "d" _RESET_ : "-");
    printf((perms & S_IRUSR) ? "r" : "-");
    printf((perms & S_IWUSR) ? "w" : "-");
    printf((perms & S_IXUSR) ? "x" : "-");
    printf((perms & S_IRGRP) ? "r" : "-");
    printf((perms & S_IWGRP) ? "w" : "-");
    printf((perms & S_IXGRP) ? "x" : "-");
    printf((perms & S_IROTH) ? "r" : "-");
    printf((perms & S_IWOTH) ? "w" : "-");
    printf((perms & S_IXOTH) ? "x" : "-");
}

void peek_execute(const char* dir_arg, const char* home_dir, bool list_long, bool show_hidden) {
    char target_dir_path[MAX_PATH_LEN];
    if (!resolve_peek_path(dir_arg, home_dir, target_dir_path, sizeof(target_dir_path))) {
        return;
    }

    struct dirent **name_list;
    int count = scandir(target_dir_path, &name_list, NULL, alphasort);

    if (count < 0) {
        fprintf(stderr, _RED_ "Shell Error: " _RESET_ "peek: Could not scan directory '%s': %s\n",
                target_dir_path, strerror(errno));
        return;
    }

    if (list_long) {
        long int total_blocks = 0;
        for (int i = 0; i < count; i++) {
            if (!show_hidden && name_list[i]->d_name[0] == '.') {
                continue;
            }
            char item_full_path[MAX_PATH_LEN * 2]; // Path + name
            snprintf(item_full_path, sizeof(item_full_path), "%s/%s", target_dir_path, name_list[i]->d_name);
            struct stat item_stat;
            if (lstat(item_full_path, &item_stat) == 0) { // Use lstat for symlinks
                total_blocks += item_stat.st_blocks;
            }
        }
        printf("total %ld\n", total_blocks / 2); // st_blocks often in 512-byte units
    }

    for (int i = 0; i < count; i++) {
        if (!show_hidden && name_list[i]->d_name[0] == '.') {
            free(name_list[i]);
            continue;
        }

        char item_full_path[MAX_PATH_LEN * 2];
        snprintf(item_full_path, sizeof(item_full_path), "%s/%s", target_dir_path, name_list[i]->d_name);
        struct stat item_stat;

        if (lstat(item_full_path, &item_stat) == -1) {
            fprintf(stderr, _RED_ "Shell Error: " _RESET_ "peek: lstat failed for '%s': %s\n",
                    item_full_path, strerror(errno));
            free(name_list[i]);
            continue;
        }

        if (list_long) {
            print_file_permissions(item_stat.st_mode);

            printf(" %2ld", (long)item_stat.st_nlink);

            struct passwd* owner_info = getpwuid(item_stat.st_uid);
            printf(_YELLOW_ " %s" _RESET_, owner_info ? owner_info->pw_name : "UNKNOWN");

            struct group* group_info = getgrgid(item_stat.st_gid);
            printf(_YELLOW_ " %s" _RESET_, group_info ? group_info->gr_name : "UNKNOWN");

            printf(" %7ld", (long)item_stat.st_size);

            char time_buf[80];
            strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", localtime(&item_stat.st_mtime));
            printf(" %s ", time_buf);
        }

        // Coloring and name printing
        if (S_ISDIR(item_stat.st_mode)) {
            printf(_BLUE_ "%s\n" _RESET_, name_list[i]->d_name);
        } else if (S_ISLNK(item_stat.st_mode) && list_long) {
             char link_target[MAX_PATH_LEN];
             ssize_t len = readlink(item_full_path, link_target, sizeof(link_target) - 1);
             if (len != -1) {
                 link_target[len] = '\0';
                 printf(_CYAN_ "%s" _RESET_ " -> %s\n", name_list[i]->d_name, link_target);
             } else {
                 printf(_CYAN_ "%s\n" _RESET_, name_list[i]->d_name);
             }
        } else if (S_ISLNK(item_stat.st_mode)) { // Short listing for symlink
            printf(_CYAN_ "%s\n" _RESET_, name_list[i]->d_name);
        }
         else if (item_stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) { // Executable
            printf(_GREEN_ "%s\n" _RESET_, name_list[i]->d_name);
        } else {
            printf("%s\n", name_list[i]->d_name); // Regular file or other
        }
        free(name_list[i]);
    }
    free(name_list);
}