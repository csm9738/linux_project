#!/bin/bash

source "$(dirname "$0")/modules/utils.sh"

save_current_pwd() {
    mkdir -p "$PROJECT_ROOT/build"
    pwd > "$PROJECT_ROOT/build/last_run_dir.txt"
}

save_current_pwd

show_logo

if [ "$#" -eq 0 ]; then
    GIT_LOG_FILE="$PROJECT_ROOT/build/git_log.txt"
    git --no-pager log --graph --all --pretty=format:'%h -%d%s (%ar) <%an>' > "$GIT_LOG_FILE"
    
    "$PROJECT_ROOT/src/lib/ui" "$GIT_LOG_FILE"
    EXIT_CODE=$?

    if [ "$EXIT_CODE" -eq 2 ]; then
        clear
        ./tests/run_tests.sh
    elif [ "$EXIT_CODE" -eq 3 ]; then
        clear
        ./src/modules/commit.sh
    fi
fi
