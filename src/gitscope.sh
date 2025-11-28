#!/bin/bash

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
SCRIPT_DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

source "$PROJECT_ROOT/src/modules/utils.sh"

save_current_pwd() {
    mkdir -p "$PROJECT_ROOT/build"
    pwd > "$PROJECT_ROOT/build/last_run_dir.txt"
}

save_current_pwd

show_logo

if [ "$#" -eq 0 ]; then
    GIT_LOG_FILE="$PROJECT_ROOT/build/git_log.txt"
    git --no-pager log --graph --all > "$GIT_LOG_FILE"
    
    "$PROJECT_ROOT/build/ui" "$GIT_LOG_FILE"
    EXIT_CODE=$?

    if [ "$EXIT_CODE" -eq 2 ]; then
        clear
        "$PROJECT_ROOT/tests/run_tests.sh"
    elif [ "$EXIT_CODE" -eq 3 ]; then
        clear
        "$PROJECT_ROOT/src/modules/commit.sh"
    elif [ "$EXIT_CODE" -eq 4 ]; then
        clear
        "$PROJECT_ROOT/src/modules/customize_tree.sh"
    fi
fi