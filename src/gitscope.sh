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

COMMAND=$1
shift

case $COMMAND in
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
        git --no-pager log --graph --all > "$GIT_LOG_FILE"
        
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
        elif [ "$EXIT_CODE" -eq 3 ]; then
            clear
            "$PROJECT_ROOT/src/modules/commit.sh"
        elif [ "$EXIT_CODE" -eq 4 ]; then
            clear
            exec "$0"
        fi
        ;;
esac