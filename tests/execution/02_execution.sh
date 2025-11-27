#!/bin/bash

BASE_DIR=$(dirname "$0")/../..
cd "$BASE_DIR" || exit 1

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "Running test: test_gitscope_execution.sh"

BUILD_SCRIPT="build/gitscope.sh"
if [ ! -f "$BUILD_SCRIPT" ]; then
    echo "  [1/1] Running gitscope.sh execution test... ${RED}FAIL${NC}"
    echo "    - Prerequisite failed: '$BUILD_SCRIPT' does not exist. Run 'test_install_script.sh' first."
    exit 1
fi

echo -n "  [1/1] Running gitscope.sh execution test... "

PATH="$PATH" COLUMNS=80 LINES=24 timeout 1s script -q /dev/null ./$BUILD_SCRIPT > /dev/null 2>&1

EXIT_CODE=$?
if [ "$EXIT_CODE" -eq 124 ]; then
    echo -e "${GREEN}PASS${NC}"
elif [ "$EXIT_CODE" -eq 0 ]; then
    echo -e "${RED}FAIL${NC}"
    echo "    - '$BUILD_SCRIPT' execution failed: exited with 0 (expected 124 for ncurses UI). This might indicate the UI did not start correctly."
    exit 1
else
    echo -e "${RED}FAIL${NC}"
    echo "    - '$BUILD_SCRIPT' execution failed unexpectedly. Exit code: $EXIT_CODE."
    exit 1
fi

echo "Test finished successfully."
