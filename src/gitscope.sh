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

show_logo
shift

case $1 in
    customize)
        "$PROJECT_ROOT/src/modules/customize_tree.sh"
        ;;
    test)
        "$PROJECT_ROOT/tests/run_tests.sh"
        ;;
    commit)
        "$PROJECT_ROOT/src/modules/commit.sh"
        ;;
    *)
        GIT_LOG_FILE="$PROJECT_ROOT/build/git_log.txt"
        mkdir -p "$PROJECT_ROOT/build"
        # include color and decoration so saved log contains ANSI escapes for branches
        git --no-pager log --graph --all --decorate=short --color=always > "$GIT_LOG_FILE"
        
        CONFIG_FILE="$PROJECT_ROOT/config/gitscope.conf"
        if [ -f "$CONFIG_FILE" ]; then
            source "$CONFIG_FILE"
            export LINE_STYLE
        fi

        "$PROJECT_ROOT/build/ui" "$GIT_LOG_FILE" "$PROJECT_ROOT"
        EXIT_CODE=$?

        if [ "$EXIT_CODE" -eq 2 ]; then
            clear
            "$PROJECT_ROOT/tests/run_tests.sh"
            exec "$0" # Re-launch UI after running tests
        elif [ "$EXIT_CODE" -eq 3 ]; then
            clear
            # Read commit type and message from temporary files
            COMMIT_TYPE=""
            COMMIT_MESSAGE=""
            if [ -f "/tmp/gitscope_commit_type.tmp" ]; then
                COMMIT_TYPE=$(cat "/tmp/gitscope_commit_type.tmp")
                rm "/tmp/gitscope_commit_type.tmp"
            fi
            if [ -f "/tmp/gitscope_commit_message.tmp" ]; then
                COMMIT_MESSAGE=$(cat "/tmp/gitscope_commit_message.tmp")
                rm "/tmp/gitscope_commit_message.tmp"
            fi
            
            # Execute commit.sh with type and message
            if [ -n "$COMMIT_TYPE" ] && [ -n "$COMMIT_MESSAGE" ]; then
                "$PROJECT_ROOT/src/modules/commit.sh" "$COMMIT_TYPE" "$COMMIT_MESSAGE"
            else
                echo "Error: Commit type or message not found from UI. Aborting commit."
            fi
            exec "$0" # Re-launch UI after commit
        elif [ "$EXIT_CODE" -eq 4 ]; then
            clear
            exec "$0"
        fi
        ;;
esac