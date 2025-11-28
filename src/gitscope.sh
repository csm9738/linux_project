#!/bin/bash

# GitScope - Main Entry Point

# --- Configuration ---
UI_BINARY="src/lib/ui"
GIT_LOG_ARGS=(
    --graph 
    --all 
    --oneline 
    --decorate 
    "--pretty=format:%h - %an, %ar : %s"
)
TMP_LOG_FILE=$(mktemp)

# --- Cleanup Function ---
# Ensures the temporary log file is removed on exit
cleanup() {
    rm -f "$TMP_LOG_FILE"
}
trap cleanup EXIT

# --- Main Logic ---

# 1. Check if UI binary exists
if [ ! -f "$UI_BINARY" ]; then
    echo "Error: UI binary not found at '$UI_BINARY'."
    echo "Please run 'make' to compile the project."
    exit 1
fi

# 2. Generate the git log and store it in a temporary file
git log "${GIT_LOG_ARGS[@]}" > "$TMP_LOG_FILE"
if [ ! -s "$TMP_LOG_FILE" ]; then
    echo "Error: Failed to generate git log or git history is empty."
    exit 1
fi

# 3. Run the ncurses UI with the log file
"$UI_BINARY" "$TMP_LOG_FILE"
exit_code=$?

# 4. Handle exit codes from the UI
case $exit_code in
    0)
        echo "GitScope exited normally."
        ;;
    2)
        echo "Switching to Test Runner..."
        # ./scripts/run_tests.sh
        ;;
    3)
        echo "Switching to Commit Tool..."
        # ./src/modules/commit.sh
        ;;
    *)
        echo "GitScope exited with an unknown code: $exit_code"
        ;;
esac