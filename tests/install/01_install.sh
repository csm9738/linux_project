#!/bin/bash

BASE_DIR=$(dirname "$0")/../..
cd "$BASE_DIR" || exit 1

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "Running test: test_install_process"

echo -n "  [1/5] Cleaning up environment... "
rm -rf build src/lib
mkdir -p build src/lib
echo -e "${GREEN}DONE${NC}"

echo -n "  [2/5] Compiling C code... "
gcc -Wall -Isrc/core -o src/lib/ui src/core/ui.c -lncurses
if [ $? -eq 0 ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
    exit 1
fi

echo -n "  [3/5] Copying and modifying script... "
cp src/gitscope.sh build/gitscope.sh
sed -i.bak 's|source "$(dirname "$0")/modules/utils.sh"|source "$(dirname "$0")/../src/modules/utils.sh"|g' build/gitscope.sh
if [ $? -eq 0 ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
    exit 1
fi

echo -n "  [4/5] Setting execute permissions... "
chmod +x build/gitscope.sh
if [ -x "build/gitscope.sh" ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
    exit 1
fi

echo -n "  [5/5] Verifying final files... "
BUILD_SCRIPT="./build/gitscope.sh"
C_BINARY="src/lib/ui"
if [ -f "$BUILD_SCRIPT" ] && [ -f "$C_BINARY" ] && grep -q 'source "$(dirname "$0")/../src/modules/utils.sh"' "$BUILD_SCRIPT"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
    exit 1
fi

echo "Test finished successfully."