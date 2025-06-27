# Compiler and flags
CC = gcc
# CFLAGS for compilation: Wall (all warnings), Wextra (extra warnings), g (debug symbols), Iinclude (header directory)
CFLAGS = -Wall -Wextra -g -Iinclude
# LDFLAGS for linking (if any special libraries were needed)
LDFLAGS =

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = .

# Executable name
TARGET = $(BIN_DIR)/shellby

# --- Source File Discovery ---
# Use the 'find' command to recursively locate all .c files in the source directory.
# This is robust and automatically adapts to new files/subdirectories.
SRCS = $(shell find $(SRC_DIR) -name '*.c')

# --- Object File Mapping ---
# Generate corresponding .o file paths in OBJ_DIR, preserving the subdirectory structure.
# e.g., 'src/core/parser.c' becomes 'obj/core/parser.o'
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# --- Build Rules ---

# Default target: build the executable. This is the first rule.
all: $(TARGET)

# Rule to link the executable from all object files.
# $@ is the target name ('shellby')
# $^ are all the prerequisites (all .o files)
$(TARGET): $(OBJS)
	@echo "Linking..."
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Shell '$(TARGET)' built successfully."

# Rule to compile a .c source file into a .o object file.
# This pattern rule handles all object files in any subdirectory of obj/.
# $< is the first prerequisite (the .c file)
# $@ is the target name (the .o file)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	# Create the destination directory for the object file if it doesn't exist.
	# $(@D) is an automatic variable for the directory part of the target name.
	# e.g., for 'obj/core/parser.o', $(@D) is 'obj/core'
	@mkdir -p $(@D)
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c -o $@ $<

# --- Housekeeping Rules ---

# Clean up all build artifacts.
clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)
	rm -f .shellby_history.txt
	@echo "Cleanup complete."

# Placeholder test target from your original Makefile.
# Note: 'test.c' is not part of the provided project files.
test:
	$(CC) -fsanitize=address,undefined -g test.c -o test_run

# Phony targets are not actual files.
.PHONY: all clean test