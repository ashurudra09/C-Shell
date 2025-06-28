# Shellby - A Custom Shell in C

Shellby is a lightweight, custom shell implementation written in C. It emulates a core subset of functionalities found in standard Unix shells like `bash`, including built-in commands, command history, I/O redirection, and background process management.

---

## Table of Contents
- [Shellby - A Custom Shell in C](#shellby---a-custom-shell-in-c)
  - [Table of Contents](#table-of-contents)
  - [Prerequisites](#prerequisites)
  - [Getting Started](#getting-started)
  - [Project Structure](#project-structure)
  - [Testing](#testing)
    - [How to Run the Tests](#how-to-run-the-tests)
  - [Features](#features)
    - [1) Core Shell Functionality](#1-core-shell-functionality)
    - [2) Piping and I/O Redirection](#2-piping-and-io-redirection)
    - [3) `warp`](#3-warp)
    - [4) `peek`](#4-peek)
    - [5) `pastevents`](#5-pastevents)
    - [6) `proclore`](#6-proclore)
    - [7) `seek`](#7-seek)
    - [8) `iman`](#8-iman)
    - [9) `activities`](#9-activities)
    - [10) `ping`](#10-ping)
    - [11) `neonate`](#11-neonate)
    - [12) Background Processes](#12-background-processes)
    - [13) Job Control and Signals](#13-job-control-and-signals)
    - [14) `fg` and `bg`](#14-fg-and-bg)
  - [Key Design Features](#key-design-features)
  - [Limitations](#limitations)
  - [Future Scope](#future-scope)

---

## Prerequisites

*   A C compiler like `gcc` (GNU Compiler Collection)
*   The `make` build automation tool

These can be installed via your system's package manager (e.g., `sudo apt install build-essential` on Debian/Ubuntu).

---

## Getting Started

**1. Clone the repository:**
```bash
git clone git@github.com:ashurudra09/Shellby.git
cd Shellby
```

**2. Compile the project:**
```bash
make
```

**3. Run Shellby:**
The executable will be named `shellby`.
```bash
./shellby
```
The directory where Shellby is launched is treated as its "home" (`~`) directory for prompt display and path expansion.

**4. Clean up build artifacts:**
This removes the executable, all object files, and the shell's history file.
```bash
make clean
```

---

## Project Structure

The codebase is organized into a modular structure to separate concerns and improve maintainability.

```
.
├── include/            # Public headers for all modules
│   ├── commands/       # Headers for built-in command implementations
│   ├── core/           # Headers for the shell's core logic
│   └── utils/          # Headers for utility modules (history queue, etc.)
│
├── src/                # Source code implementations
│   ├── commands/       # .c files for built-in commands (peek, warp, etc.)
│   ├── core/           # .c files for core logic (parser, executor, etc.)
│   ├── utils/          # .c files for utility modules
│   └── main.c          # Main entry point and the primary shell loop
│
├── Makefile            # Build script for compiling the project
└── shellby             # The final executable (after running make)
```

---

## Testing

A test script, `test_shellby.sh`, is included to verify all major functionalities of the shell.

### How to Run the Tests

1.  First, ensure the project is compiled:
    ```bash
    make
    ```
2.  Make the test script executable:
    ```bash
    chmod +x test_shellby.sh
    ```
3.  Run the script:
    ```bash
    ./test_shellby.sh
    ```

It creates a temporary `shellby_test_environment/` directory for its operations and cleans it up upon completion. Follow the on-screen prompts to proceed through each test case and observe the output.

---

## Features

### 1) Core Shell Functionality
*   **Interactive Prompt:** Displays user, system, and the current path relative to the shell's home (`~`). Shows the execution time of commands that take longer than 1 second.
*   **Command Chaining:** Supports multiple commands on a single line, separated by semicolons (`;`).
    ```bash
    <user@system:~> command1 ; command2 ; command3
    ```
*   **External Command Execution:** Executes any command found in the system's `PATH` (e.g., `ls`, `grep`, `gcc`).

### 2) Piping and I/O Redirection
Shellby supports connecting commands with pipes and redirecting their standard input and output.

*   **Piping (`|`):** The standard output of the command on the left is connected to the standard input of the command on the right.
    ```bash
    # List all files, then count the lines containing ".c"
    <user@system:~> ls -l | grep .c | wc -l
    ```

*   **I/O Redirection:** You can redirect `stdin`, `stdout`, and append to files.

| Operator | Description                               | Example                               |
| :---     | :---------------------------------------- | :------------------------------------ |
| `>`      | Redirect standard output to a file (overwrite). | `echo "Hello" > output.txt`           |
| `>>`     | Redirect standard output to a file (append).    | `echo "World" >> output.txt`          |
| `<`      | Redirect standard input from a file.      | `sort < output.txt`                   |

*   Piping and redirection can be combined:
    ```bash
    # Read from input.txt, find lines with 'error', and save to error_log.txt
    <user@system:~> cat < input.txt | grep "error" > error_log.txt
    ```

### 3) `warp`
Changes the current working directory, similar to `cd`.
*   **Syntax:** `warp [<path1>] [<path2>] ...`
*   **Functionality:**
    *   `warp` or `warp ~`: Navigates to the shell's startup directory.
    *   `warp -`: Navigates to the previous working directory (OLDPWD).
    *   `warp ..`: Navigates to the parent directory.
    *   Supports multiple arguments, changing into each directory sequentially.

### 4) `peek`
Lists files and directories, similar to `ls`. Output is sorted lexicographically.
*   **Syntax:** `peek [<flags>] [<path>]`
*   **Functionality:** Lists contents of the current directory by default. Hidden files (starting with `.`) are omitted unless the `-a` flag is used.

| Flag | Description                                                  |
| :--- | :----------------------------------------------------------- |
| `-l` | Displays a detailed long listing (permissions, owner, size, etc.). |
| `-a` | Shows all entries, including hidden files.                   |

**Note:** *Flags can be combined (e.g., `peek -la`).*

*   **Output Coloring:**
    *   **Blue:** Directories
    *   **Green:** Executable files
    *   **Cyan:** Symbolic links

### 5) `pastevents`
Manages and re-executes commands from a persistent history.
*   **Functionality:** Stores the last 15 unique commands in `.shellby_history.txt`. History is loaded on startup and saved on exit.

| Command                     | Description                                                  |
| :-------------------------- | :----------------------------------------------------------- |
| `pastevents`                | Displays the command history, from oldest to newest.         |
| `pastevents purge`          | Clears all commands from history (in-memory and on-disk).    |
| `pastevents execute <index>`| Executes the command at the given index (1 = most recent).   |

*   **Note:** using `up-arrow` and `down-arrow` shift between past commands using the same list.

### 6) `proclore`
Displays information about a process.
*   **Syntax:** `proclore [<pid>]`
*   **Functionality:** If `<pid>` is omitted, it displays information for the Shellby process itself.
*   **Information Displayed:**
    *   **PID:** The Process ID.
    *   **State:** Process state (e.g., `R` for Running, `S` for Sleeping). A `+` indicates it's a foreground process.
    *   **Process Group:** The Process Group ID.
    *   **Virtual Memory:** The virtual memory size consumed.
    *   **Executable Path:** The absolute path to the executable, shortened with `~` if inside the shell's home.

### 7) `seek`
Recursively searches for files or directories.
*   **Syntax:** `seek [<flags>] <target_name> [<directory>]`
*   **Functionality:** Performs an exact name match for `<target_name>`. Searches the current directory by default.

| Flag | Description                                                  |
| :--- | :----------------------------------------------------------- |
| `-d` | Search for **directories** only.                             |
| `-f` | Search for **files** only. (Mutually exclusive with `-d`).   |
| `-e` | Executes an action if **only one match** is found. If it's a directory, `warp`s to it. If it's a file, prints its contents. If multiple matches are found, this flag does nothing. |

*   **Output Coloring:**
    *   **Blue:** Matched directories
    *   **Green:** Matched files

### 8) `iman`
Fetches and displays manual pages for commands, similar to `man`.
*   **Syntax:** `iman <command_name>`
*   **Functionality:** Makes an HTTP GET request to an online manual page repository (`man.he.net`), parses the HTML response, and prints the plain-text content of the man page to the terminal. This command requires an active internet connection to function.

### 9) `activities`
Displays a list of all processes that have been spawned by the shell and are currently running or stopped.
*   **Syntax:** `activities`
*   **Functionality:** Scans the system's process table and identifies all child processes belonging to the current shell session. The output is sorted by Process ID (PID).
*   **Output Format:** `[pid]: [command name] - [state]`
    *   **State:** Can be `Running` (for running or sleeping processes) or `Stopped` (for suspended or zombie processes).

### 10) `ping`
Sends a specified signal to a process.
*   **Syntax:** `ping <pid> <signal_number>`
*   **Functionality:** A wrapper around the `kill` system call. It sends the signal identified by `<signal_number>` to the process with the ID `<pid>`.
*   **Example:** To terminate a process with PID 12345, you can send signal 9 (SIGKILL).
    ```bash
    <user@system:~> ping 12345 9
    ```

### 11) `neonate`
Periodically prints the process ID of the most recently created process on the system.
*   **Syntax:** `neonate -n <time_arg>`
*   **Functionality:** Enters a loop that prints the latest PID found in `/proc` every `<time_arg>` seconds. The command actively listens for keyboard input and will terminate when the `x` key is pressed. This demonstrates non-blocking I/O by manipulating terminal settings.

### 12) Background Processes
Run any external command in the background by appending `&`.
*   **Functionality:** The shell immediately returns to the prompt after launching the process. A notification is printed when the process starts and when it terminates.
    ```bash
    <user@system:~> sleep 5 &
    Shellby: Started background process [1] sleep (PID 12345)
    <user@system:~> # Prompt returns instantly
    # ... after 5 seconds ...
    Shellby: Background process 'sleep' (PID 12345) exited normally with status 0.
    ```
### 13) Job Control and Signals
Shellby provides robust job control for managing foreground and background processes.

*   **`Ctrl+C` (SIGINT):** Interrupts the currently running foreground process. If no process is running, it does nothing.
*   **`Ctrl+Z` (SIGTSTP):** Suspends the currently running foreground process and moves it to the background with a "Stopped" state.
*   **`Ctrl+D` (EOF):** Logs out of the shell. Before exiting, it sends a kill signal to all background jobs to ensure a clean shutdown.

### 14) `fg` and `bg`
Provides job control to manage background processes.

*   **`fg <pid>`**
    *   Brings a running or stopped background job to the foreground.
    *   The shell gives terminal control to the process and waits for it to complete or be stopped.

*   **`bg <pid>`**
    *   Resumes a *stopped* background job, allowing it to run in the background.
    *   The shell does not wait for the command and returns to the prompt immediately.

---

## Key Design Features

*   **Modular Architecture:** The codebase is organized into a clean, hierarchical structure. Core logic (parsing, execution), built-in command implementations, and utilities are separated into distinct modules, promoting readability and maintainability.
*   **Centralized State Management:** A `ShellState` struct holds all global shell information (e.g., current directories, history, background processes), preventing the need to pass many arguments between functions.
*   **Persistent History:** Command history is saved to a file and reloaded across sessions.
*   **Resource Management:** All dynamically allocated memory is intended to be freed on exit to prevent memory leaks.

---

## Limitations

*   **Built-in Commands in Pipelines:** Built-in commands (like `warp`, `peek`, `seek` etc) cannot be used in a pipeline. They are only executed if they are the sole command on the line (or in a semicolon-separated list).
*   **`iman` Command:** Requires an active internet connection to fetch manual pages, unlike the system `man` command which uses local files.
*   **Strict Spacing for Operators:** The parser requires spaces around piping and redirection operators. For example, `ls>output.txt` will fail, whereas `ls > output.txt` will succeed.
*   **No `stderr` Redirection:** Only `stdin` and `stdout` can be redirected. `stderr` redirection (e.g., `2>`) is not supported.
*   **Argument Parsing:** Does not handle arguments with spaces (e.g., `"a file with spaces.txt"`).

---

## Future Scope

*   Support for Built-in Commands in Pipelines.
*   Support for `stderr` Redirection (e.g., `2>`).
*   Introduce tab completion for commands and file paths.
*   Enhance the parser to handle quoted arguments and remove strict spacing requirements.
 
---
