#!/bin/bash

# 테스트 환경 설정
BASE_DIR=$(dirname "$0")/..
cd "$BASE_DIR" || exit 1

# 색상 코드
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# --- 테스트 시작 ---
echo "Running test: test_install_script.sh"

# 1. 이전 빌드 결과물 정리
echo -n "  [1/4] Cleaning up previous builds... "
rm -rf build
rm -rf src/lib
mkdir -p build
mkdir -p src/lib
if [ -d "build" ] && [ -d "src/lib" ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
    echo "    - Failed to create build or src/lib directory."
    exit 1
fi

# 2. install.sh 실행
echo -n "  [2/4] Running install.sh... "
./install.sh > /dev/null 2>&1
echo -e "${GREEN}DONE${NC}"

echo "    --- Debugging: Directory Contents after install.sh ---"
ls -l build/
ls -l src/lib/
echo "    -----------------------------------------------------"


# 3. 결과물 검증
echo -n "  [3/4] Verifying generated files... "
BUILD_SCRIPT="build/gitscope.sh"
C_BINARY="src/lib/ui"

if [ -f "$BUILD_SCRIPT" ] && [ -f "$C_BINARY" ] && [ -x "$BUILD_SCRIPT" ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
    echo "    - Check if '$BUILD_SCRIPT' and '$C_BINARY' exist and '$BUILD_SCRIPT' is executable."
    exit 1
fi

# 4. 경로 수정 검증
echo -n "  [4/4] Verifying path modification in build script... "
EXPECTED_PATH='source "$(dirname "$0")/../src/modules/utils.sh"'
if grep -q "$EXPECTED_PATH" "$BUILD_SCRIPT"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
    echo "    - The source path in '$BUILD_SCRIPT' was not modified correctly."
    exit 1
fi

echo "Test finished successfully."
