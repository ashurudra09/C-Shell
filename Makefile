# Makefile for the Custom Shell

CC = gcc
# CFLAGS for compilation: Wall (all warnings), Wextra (extra warnings), g (debug symbols), Iinclude (header directory)
CFLAGS = -Wall -Wextra -g -Iinclude
# LDFLAGS for linking (if any special libraries were needed, e.g., -lm for math)
LDFLAGS =

# Directories
SRC_DIR = src
OBJ_DIR = obj
# Executable name
TARGET = shellby

# Automatically find all .c files in SRC_DIR
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Generate corresponding .o file paths in OBJ_DIR
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Default target: build the executable
all: $(TARGET)

# Link the executable from object files
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Shell '$(TARGET)' built successfully."

# Rule to compile .c source files into .o object files
# $< is the first prerequisite (the .c file)
# $@ is the target name (the .o file)
# | $(OBJ_DIR) makes $(OBJ_DIR) an order-only prerequisite, ensuring it's created before compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create the object directory if it doesn't exist
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@echo "Created directory $(OBJ_DIR)."

# Clean up build artifacts
clean:
	@echo "Cleaning up..."
	rm -f $(TARGET) $(OBJ_DIR)/*.o
	rm -rf $(OBJ_DIR)
	@echo "Cleanup complete."

# Test target (assuming test.c is in the root directory)
# Note: 'test.c' is not provided, so this is a placeholder.
test:
	$(CC) -fsanitize=address,undefined -g test.c -o test_run

# Phony targets are not actual files
.PHONY: all clean test