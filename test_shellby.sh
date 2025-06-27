#!/bin/bash

# ANSI Color Codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# --- Test Environment Setup ---
TEST_DIR="shellby_test_environment"

# Function to set up the directory structure for testing
setup_test_environment() {
    echo -e "${YELLOW}Setting up test environment in './${TEST_DIR}'...${NC}"
    rm -rf "$TEST_DIR"
    mkdir -p "$TEST_DIR/dir1/subdir1"
    mkdir -p "$TEST_DIR/dir2"

    # Create files for peek and seek
    echo "This is file1." > "$TEST_DIR/file1.txt"
    touch "$TEST_DIR/dir1/file2.log"
    echo "This is a hidden file." > "$TEST_DIR/.hidden_file"
    
    # Create an executable file
    echo -e '#!/bin/bash\necho "Executable script ran!"' > "$TEST_DIR/executable.sh"
    chmod +x "$TEST_DIR/executable.sh"

    # Create a duplicate name for testing seek -e with multiple matches
    mkdir -p "$TEST_DIR/dir1/subdir1/unique_dir"
    touch "$TEST_DIR/dir2/unique_file.txt"
    echo "unique file content" > "$TEST_DIR/dir2/unique_file.txt"
    
    echo "Setup complete."
}

# Function to clean up the test environment
cleanup() {
    echo -e "\n${YELLOW}Cleaning up test environment...${NC}"
    rm -rf "$TEST_DIR"
    rm -f .shellby_history.txt
    echo "Cleanup complete."
}

# Ensure cleanup happens on exit
trap cleanup EXIT

# --- Main Test Execution ---

# Check if shellby executable exists
if [ ! -f ./shellby ]; then
    echo -e "${RED}Error: 'shellby' executable not found. Please run 'make' first.${NC}"
    exit 1
fi

setup_test_environment

# --- Test Cases ---

echo -e "\n${BLUE}========================================="
echo -e "  STARTING SHELLBY FUNCTIONALITY TESTS"
echo -e "=========================================${NC}"
echo "NOTE: Shellby's '~' refers to the directory it was started from."
echo "In this script, '~' will be the project's root directory."
read -p "Press Enter to begin..."

# 1. Testing 'warp' command
echo -e "\n${GREEN}--- 1. Testing 'warp' (Directory Navigation) ---${NC}"
echo "Expected behavior:"
echo "1. Start in project root."
echo "2. Warp to '$TEST_DIR/dir1', print path."
echo "3. Warp to '..', then 'dir2' (sequentially), print final path."
echo "4. Warp to previous directory ('-'), print path."
echo "5. Warp to home ('~'), print path."
echo "6. Attempt to warp to a non-existent directory, see error."
read -p "Press Enter to run 'warp' tests..."

./shellby <<EOF
pwd
warp $TEST_DIR/dir1
pwd
warp .. dir2
pwd
warp -
pwd
warp ~
pwd
warp non_existent_dir
q
EOF

read -p "Warp tests complete. Press Enter to continue..."

# 2. Testing 'peek' command
echo -e "\n${GREEN}--- 2. Testing 'peek' (Directory Listing) ---${NC}"
echo "Expected behavior:"
echo "1. Basic 'peek' in '$TEST_DIR'."
echo "2. 'peek -a' shows '.hidden_file'."
echo "3. 'peek -l' shows detailed listing."
echo "4. 'peek -la' shows detailed listing including hidden files."
echo "5. 'peek ~' lists the project root directory."
read -p "Press Enter to run 'peek' tests..."

cd "$TEST_DIR" # Change directory for simpler peek commands
../shellby <<EOF
peek
peek -a
peek -l
peek -la
peek ~
q
EOF
cd .. # Go back to project root

read -p "Peek tests complete. Press Enter to continue..."

# 3. Testing 'seek' command
echo -e "\n${GREEN}--- 3. Testing 'seek' (File & Directory Search) ---${NC}"
echo "Expected behavior:"
echo "1. Find 'file1.txt' in '$TEST_DIR'."
echo "2. Find only directories named 'subdir1'."
echo "3. Find only files named 'file2.log'."
echo "4. Fail to find anything when using -d and -f together."
echo "5. Find and print contents of 'unique_file.txt' using '-e'."
echo "6. Find and warp to 'unique_dir' using '-e', then print the new path."
echo "7. Find a non-existent file, get 'No match found'."
read -p "Press Enter to run 'seek' tests..."

./shellby <<EOF
seek file1.txt $TEST_DIR
seek -d subdir1 $TEST_DIR
seek -f file2.log $TEST_DIR
seek -d -f file1.txt $TEST_DIR
seek -e unique_file.txt $TEST_DIR
seek -e unique_dir $TEST_DIR
pwd
seek non_existent_file.xyz $TEST_DIR
q
EOF

read -p "Seek tests complete. Press Enter to continue..."

# 4. Testing 'proclore' and Background Processes
echo -e "\n${GREEN}--- 4. Testing 'proclore', Background Processes & Timed Prompt ---${NC}"
echo "This part is interactive."
echo -e "\n${YELLOW}Step 4a: Test 'proclore'${NC}"
echo "Expected behavior:"
echo "1. 'proclore' will show info for the shellby process itself."
echo "2. 'proclore $$' will show info for the parent bash script process."
read -p "Press Enter to run 'proclore' test..."

./shellby <<EOF
proclore
proclore $$
q
EOF

echo -e "\n${YELLOW}Step 4b: Test Background Processes & Timed Prompt${NC}"
echo "Expected behavior:"
echo "1. You will be dropped into an interactive shellby session."
echo "2. A 'sleep 3 &' command will start a background process."
echo "   - You should see a 'Started background process' message."
echo "   - The prompt should return immediately."
echo "3. After ~3 seconds, you should see a 'Background process ... exited' message."
echo "4. A 'sleep 2' (foreground) command will run."
echo "   - The prompt will be blocked for 2 seconds."
echo "   - The next prompt will show the command took >1s (e.g., 'sleep: 2s')."
echo -e "\n${BLUE}INSTRUCTIONS: When you are in the shellby prompt, please type the following commands:"
echo "1. sleep 3 &"
echo "2. (wait for the exit message)"
echo "3. sleep 2"
echo "4. q (to exit and continue the test script)"
read -p "Press Enter to start the interactive session..."

./shellby

read -p "Interactive tests complete. Press Enter to continue..."

# 5. Testing 'pastevents' and History
echo -e "\n${GREEN}--- 5. Testing 'pastevents' (Command History) ---${NC}"
echo -e "\n${YELLOW}Step 5a: Basic History and Execution${NC}"
echo "Expected behavior:"
echo "1. 'pastevents' is initially empty."
echo "2. After running commands, 'pastevents' shows them."
echo "3. 'pastevents execute 1' runs the last command ('pwd')."
echo "4. History should now contain 'pwd' twice."
echo "5. 'pastevents purge' clears the history."
read -p "Press Enter to run basic 'pastevents' tests..."

rm -f .shellby_history.txt

./shellby <<EOF
pastevents
echo First command
pwd
pastevents
pastevents execute 1
pastevents
pastevents purge
pastevents
q
EOF

echo -e "\n${YELLOW}Step 5b: History Persistence Across Sessions${NC}"
echo "Expected behavior:"
echo "1. We will run two commands and exit."
echo "2. We will restart shellby."
echo "3. 'pastevents' should show the commands from the previous session."
read -p "Press Enter to test history persistence..."

./shellby <<EOF
ls -l
echo History should persist!
q
EOF

./shellby <<EOF
pastevents
q
EOF

read -p "History tests complete. Press Enter to continue..."


# --- NEW TEST SECTION ---
# 6. Testing Arrow-Key History Navigation (Interactive)
echo -e "\n${GREEN}--- 6. Testing Arrow-Key History Navigation (Interactive) ---${NC}"
echo "This test is interactive and requires you to press the arrow keys."
echo "First, we will populate the history with a few distinct commands."

# Clean history first for a predictable state
rm -f .shellby_history.txt

# Session 1: Populate history for the interactive test
./shellby <<EOF
echo "This is the first command"
ls -F
echo "This is the third and last command"
q
EOF

echo -e "\n${YELLOW}Step 6a: Interactive Test Session${NC}"
echo "Shellby will now start. Please perform the following actions:"
echo "1. Press the ${BLUE}UP ARROW${NC}. You should see: 'echo \"This is the third and last command\"'"
echo "2. Press the ${BLUE}UP ARROW${NC} again. You should see: 'ls -F'"
echo "3. Press the ${BLUE}UP ARROW${NC} again. You should see: 'echo \"This is the first command\"'"
echo "4. Press the ${BLUE}DOWN ARROW${NC}. You should see 'ls -F' again."
echo "5. Press ${BLUE}ENTER${NC}. The 'ls -F' command should execute."
echo "6. Now, type ${BLUE}'my partial command'${NC} but DO NOT press Enter."
echo "7. Press the ${BLUE}UP ARROW${NC}. The last history item ('ls -F') should appear, replacing your text."
echo "8. Press the ${BLUE}DOWN ARROW${NC} until the prompt is empty again. You should see your typed text restored: 'my partial command'"
echo "9. Finally, type ${BLUE}q${NC} and press ${BLUE}ENTER${NC} to exit and finish the test."
read -p "Press Enter to begin the interactive test..."

# Session 2: The actual interactive test
./shellby

read -p "Interactive history test complete. Press Enter to finish."


echo -e "\n${BLUE}========================================="
echo -e "      ALL TESTS HAVE BEEN EXECUTED"
echo -e "=========================================${NC}"