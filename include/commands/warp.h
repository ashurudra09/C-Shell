#ifndef WARP_H_
#define WARP_H_

/**
 * @brief Changes the current working directory.
 *
 * Supports ".", "..", "-", "~", "~/path", and absolute/relative paths.
 * Updates prev_dir_warp with the directory before the change if successful.
 *
 * @param dir_arg The target directory argument string.
 * @param home_dir The user's home directory path.
 * @param prev_dir_warp A buffer storing the previous working directory. This function
 *                      will return the CWD *before* a successful warp.
 * @return A dynamically allocated string containing the path of the directory *before*
 *         a successful warp. The caller must free this string. Returns an empty,
 *         dynamically allocated string on failure or if OLDPWD was not set for "warp -".
 */
char* warp(const char* dir_arg, const char* home_dir, const char* prev_dir_warp);

#endif // WARP_H_