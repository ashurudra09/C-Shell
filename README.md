# Shellby - A Custom Shell in C

Shellby is a custom shell implementation written in C, providing a subset of functionalities found in standard shells like bash. It supports command execution, built-in commands for navigation and process inspection, command history, and background processes.

---

# Pre-requisites

*   `gcc` (GNU Compiler Collection)
*   `make` utility

Please install these using your system's package manager (e.g., `apt` for Debian/Ubuntu, `yum` for Fedora/CentOS, `pacman` for Arch Linux).

---

# Using Shellby

**To compile and run Shellby, navigate to the project's root directory and type:**
```bash
make
./shellby  # The executable is named shellby as per the Makefile
```

The directory in which `Shellby` is started is treated as its "home" directory for the purpose of the `~` symbol in the prompt and path expansions. This is typically the current working directory at the time of execution.

**To clean up the executable, object files, and Shellby's history file, run:**
```bash
make clean
```
This will also remove `.shellby_history.txt` if it exists in the project root (where `make clean` is run).

---

# Contents

- [Shellby - A Custom Shell in C](#shellby---a-custom-shell-in-c)
- [Pre-requisites](#pre-requisites)
- [Using Shellby](#using-shellby)
- [Contents](#contents)
- [Features](#features)
  - [Multiple Command Support](#multiple-command-support)
  - [warp](#warp)
    - [Input format:](#input-format)
    - [Example:](#example)
  - [peek](#peek)
    - [Input format:](#input-format-1)
    - [Flags:](#flags)
    - [Color Coding:](#color-coding)
  - [pastevents](#pastevents)
    - [Sub-commands:](#sub-commands)
    - [Example:](#example-1)
  - [proclore](#proclore)
    - [Input format:](#input-format-2)
    - [Information Printed:](#information-printed)
  - [seek](#seek)
    - [Input Format:](#input-format-3)
    - [Flags:](#flags-1)
    - [Color Coding of Output:](#color-coding-of-output)
  - [Background Processes \& External Commands](#background-processes--external-commands)
    - [Foreground Processes:](#foreground-processes)
    - [Background Processes:](#background-processes)
- [Improvements on Project Requirements](#improvements-on-project-requirements)
- [Assumptions](#assumptions)
- [Limitations](#limitations)
- [Future Scope](#future-scope)

---

# Features

## Multiple Command Support
Supports multiple commands separated by semicolons (`;`). Each command is executed sequentially.
```
<user@system:~> command1 ; command2 ; command3
```
Backgrounding of individual commands within a semicolon-separated list is also supported using `&` as a suffix to that specific command.
```
<user@system:~> cmd_bg1 & ; cmd_fg1 ; cmd_bg2 &
# cmd_bg1 and cmd_bg2 run in the background.
# cmd_fg1 runs in the foreground after cmd_bg1 is launched, and before cmd_bg2 is launched.
```

---

## warp
Similar to the `cd` command in Bash. Changes the current working directory.
*   Supports `.` (current directory), `..` (parent directory).
*   `~`: Refers to the directory where `Shellby` was initially started.
    *   `warp ~` or `warp` (no arguments): Changes to this initial "home" directory.
    *   `warp ~/some/path`: Changes to `some/path` relative to the initial "home" directory.
*   `-`: Changes to the previously visited directory (OLDPWD).
*   Supports both relative and absolute paths.
*   Supports multiple directory arguments; `warp` will attempt to change into each sequentially.

### Input format:
```
warp [<path1>] [<path2_relative_to_path1>] ...
```

### Example:
```
<user@system:~> warp projects
/path/to/shell_startup/projects
<user@system:~/projects> warp shellby_project
/path/to/shell_startup/projects/shellby_project
<user@system:~/projects/shellby_project> warp ~
/path/to/shell_startup
<user@system:~> warp -
/path/to/shell_startup/projects/shellby_project
<user@system:~/projects/shellby_project> warp .. src
/path/to/shell_startup/projects
/path/to/shell_startup/projects/src
<user@system:~/projects/src>
```

---

## peek
Lists files and directories, similar to `ls` in Bash. Output is in lexicographical order.
*   By default, hidden files (starting with `.`) are not shown.
*   If no path is specified, `peek` lists the contents of the current working directory.
*   Path arguments can use `.` , `..`, `~` (relative to Shellby's startup directory), or be absolute/relative paths.

### Input format:
```
peek [<flags>] [<path>]
```

### Flags:
*   `-l`: Displays detailed information (permissions, links, owner, group, size, modification time).
*   `-a`: Shows all entries, including hidden files.
*   Flags can be combined, e.g., `peek -la`, `peek -a -l`.

### Color Coding:
*   **Blue:** Directories
*   **Green:** Executable files
*   **Cyan:** Symbolic links (when using `-l`, the link target is also shown)
*   **White:** Other file types

---

## pastevents
Manages command history, similar to the `history` command.
*   Stores up to the last 15 unique command lines.
*   History is persistent across shell sessions, saved in `.shellby_history.txt` in Shellby's startup ("home") directory.
*   Does not add a command if it's identical to the immediately preceding command in history.
*   `pastevents` or `pastevents purge` commands *are* stored in history. Commands executed via `pastevents execute` result in the *executed command* being stored, not the `pastevents execute ...` line itself.

### Sub-commands:
*   **`pastevents`**: Displays the stored command history, from oldest to most recent.
*   **`pastevents purge`**: Clears all commands from the history (both in-memory and from the `.shellby_history.txt` file).
*   **`pastevents execute <index>`**: Executes the command at the specified `<index>` from the history. `index` is 1-based, where 1 is the most recent command.

### Example:
```
<user@system:~> echo hello
hello
<user@system:~> ls -a
. .. (other files)
<user@system:~> pastevents
echo hello
ls -a
<user@system:~> pastevents execute 1
. .. (other files)  # ls -a is executed
<user@system:~> pastevents
echo hello
ls -a
ls -a               # The executed command 'ls -a' is added
```

---

## proclore
Displays information about a specified process.
*   If no `<pid>` is given, it shows information for the `Shellby` process itself.

### Input format:
```
proclore [<pid>]
```

### Information Printed:
*   **pid:** Process ID.
*   **Process State:** The state of the process (e.g., `R` for Running, `S` for Sleeping). A `+` is appended if the process is in the foreground group of the terminal.
*   **Process Group:** Process Group ID.
*   **Virtual Memory:** Virtual memory size (usually in kB).
*   **Executable Path:** The absolute path to the process's executable. If the path is within `Shellby`'s startup ("home") directory, it's displayed relative to `~`.

---

## seek
Searches for files or directories recursively within a specified path.
*   Matches based on an **exact name match** for `<target_name>`.
*   If `<directory>` is not specified, searches from the current working directory.
*   Prints relative paths of matches from the base search directory.
*   If read/execute permissions are missing for directories, or read permissions for files, `seek` may skip them.
*   Prints "No match found." if no items match the criteria.

### Input Format:
```
seek [<flags>] <target_name> [<directory>]
```

### Flags:
*   `-d`: Search only for directories.
*   `-f`: Search only for files. (Cannot be used with `-d`).
*   `-e`: If exactly one match is found (respecting `-d` or `-f` if present):
    *   If the match is a directory: `warp` to that directory.
    *   If the match is a file: Print the contents of the file.
    *   If multiple matches are found, all are printed, but the `-e` action is not performed.
*   Flags can be combined (e.g., `seek -de <name> <dir>`).

### Color Coding of Output:
*   **Blue:** Matched directories
*   **Green:** Matched files

---

## Background Processes & External Commands
`Shellby` can execute external system commands that are not built-in.

### Foreground Processes:
*   Standard execution: The shell waits for the command to complete.
*   The prompt will display the time taken if the command (or sequence of commands separated by `;`) runs for more than 1 second.

### Background Processes:
*   To run an external command in the background, append `&` to it.
    ```
    <user@system:~> sleep 5 &
    Shellby: Started background process [1] sleep (PID 12345)
    <user@system:~> # Shellby prompt returns immediately
    ...
    Shellby: Background process 'sleep' (PID 12345) exited normally with status 0.
    ```
*   The shell prints the PID of the backgrounded process.
*   When a background process finishes, Shellby prints a notification message with its name, PID, and exit status.
*   Built-in commands (like `warp`, `peek`) followed by `&` will be parsed, but the `&` has no practical effect as these commands are executed synchronously by Shellby itself.

---

# Improvements on Project Requirements

*   **Graceful Exit:** The `q`, `exit`, or `quit` commands ensure `Shellby` exits cleanly, freeing dynamically allocated memory, including the history queue and its contents.
*   **History Persistence:** Command history is saved to a file (`.shellby_history.txt`) and reloaded when Shellby starts, providing persistence across sessions.
*   **Modular Design:** The codebase is organized into separate source (`.c`) and header (`.h`) files for different functionalities (prompt, warp, peek, etc.), improving readability and maintainability.
*   **Makefile:** A `Makefile` is provided for easy compilation and cleaning of build artifacts.
*   **Background Process Notification:** Clear notifications are provided when background processes start and terminate.
*   **Basic Flag Parsing:** `peek` and `seek` support combined flags (e.g., `-la` for `peek`).

---

# Assumptions

*   **`~` Interpretation:** In `Shellby`, `~` in paths and the prompt refers to the directory where the `Shellby` executable was started, not necessarily the system's user home directory (`$HOME`).
*   **Permissions for `proclore`:** `proclore` can display information for any process ID. However, without sufficient permissions (e.g., running `Shellby` as a non-root user), it might not be able to read details for processes owned by other users or the system (e.g., executable path might be inaccessible). Running `Shellby` with `sudo` is *not* required for its core operation but would grant `proclore` wider access.

---

# Limitations

*   **No Piping or I/O Redirection:** `Shellby` does not currently support command piping (`|`) or input/output redirection (`<`, `>`, `>>`).
*   **Limited Signal Handling:**
    *   `Ctrl+C`: Sends `SIGINT` to the foreground process group (standard terminal behavior). `Shellby` itself doesn't have custom handling beyond waiting for the child.
    *   `Ctrl+D`: If input is empty, logs out of `Shellby`.
    *   `Ctrl+Z`, `fg`, `bg` commands: Not implemented. There's no mechanism to stop a foreground job, move it to the background, and later resume it with `fg` or `bg`.
*   **Whitespace in Paths/Arguments:** File or directory names containing spaces are not correctly handled as single arguments due to `strtok` using space as a delimiter. Each word will be treated as a separate argument.
*   **No Shell Variables or Scripting:** Does not support environment/shell variables, aliases, or shell scripting.
*   **No Advanced Globbing:** Wildcard expansion (e.g., `*.txt`) is not supported.
*   **`seek -e` with Multiple Matches:** If `seek -e` results in multiple files or directories matching the criteria, the `-e` action (cd or cat) is *not* performed. All matches are still printed.
*   **Built-ins in Background:** While the parser accepts `&` after a built-in command, it has no practical effect as built-ins are executed synchronously within Shellby's main process.

---

# Future Scope

*   **Piping and I/O Redirection:** Implement support for `|`, `<`, `>`, and `>>`.
*   **Enhanced Signal Handling:** Implement robust handling for `Ctrl+Z` to suspend foreground jobs, and add `fg` and `bg` commands to manage these jobs.
*   **Tab Completion:** Add basic tab completion for commands and file/directory paths.
*   **Arrow Key History Navigation:** Allow users to navigate through command history using up/down arrow keys.
*   **Shell Variables & Aliases:** Introduce support for setting and using shell variables and command aliases.
*   **Improved Argument Parsing:** Handle quoted arguments to allow spaces in file names or arguments.
*   **Job Control Information:** Display a list of active background jobs (similar to `jobs` in bash).

---