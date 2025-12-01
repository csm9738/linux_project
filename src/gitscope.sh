#!/bin/bash

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
SCRIPT_DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

TOOL_ROOT="$(dirname "$SCRIPT_DIR")"

source "$TOOL_ROOT/src/modules/utils.sh"

show_logo

# optional first arg: project directory to open
TARGET_ROOT="$TOOL_ROOT"
if [ -n "$1" ] && [ -d "$1" ]; then
    TARGET_ROOT="$(cd "$1" && pwd)"
    shift
fi

case $1 in
    customize)
        "$TOOL_ROOT/src/modules/customize_tree.sh"
        ;;
    test)
        "$TOOL_ROOT/tests/run_tests.sh"
        ;;
    commit)
        "$TOOL_ROOT/src/modules/commit.sh"
        ;;
    *)
        GIT_LOG_FILE="$TARGET_ROOT/build/git_log.txt"
        mkdir -p "$TARGET_ROOT/build"
        git --no-pager -C "$TARGET_ROOT" log --graph --all --decorate=short --color=always > "$GIT_LOG_FILE"

        CONFIG_FILE="$TARGET_ROOT/config/gitscope.conf"
        if [ -f "$CONFIG_FILE" ]; then
            source "$CONFIG_FILE"
            export LINE_STYLE
        fi

        "$TOOL_ROOT/build/ui" "$GIT_LOG_FILE" "$TARGET_ROOT"
        EXIT_CODE=$?

        if [ "$EXIT_CODE" -eq 2 ]; then
            clear
            "$TOOL_ROOT/tests/run_tests.sh"
        elif [ "$EXIT_CODE" -eq 3 ]; then
            clear
            "$TOOL_ROOT/src/modules/commit.sh"
        elif [ "$EXIT_CODE" -eq 4 ]; then
            clear
            exec "$0"
        fi
        ;;
esac