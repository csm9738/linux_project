#!/bin/bash

cd "$(dirname "$0")" || exit 1

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

echo "--- Running all tests ---"

FAILURES=0

for test_script in tests/test_*.sh; do
    if [ -x "$test_script" ]; then
        "$test_script"
        if [ $? -ne 0 ]; then
            FAILURES=$((FAILURES + 1))
        fi
    else
        echo -e "${RED}ERROR: Test script '$test_script' is not executable.${NC}"
        FAILURES=$((FAILURES + 1))
    fi
done

echo "--- Test Summary ---"
if [ "$FAILURES" -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
else
    echo -e "${RED}$FAILURES test(s) failed.${NC}"
    exit 1
fi
