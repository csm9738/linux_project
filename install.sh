#!/bin/bash

# --- tmux 설치 확인 ---
if ! command -v tmux &> /dev/null; then
    echo "tmux is not installed. Attempting to install..."
    
    if [[ "$(uname)" == "Darwin" ]]; then
        if ! command -v brew &> /dev/null; then
            echo "Homebrew is not installed. Please install Homebrew first and then run this script again."
            echo "/bin/bash -c \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
            exit 1
        fi
        echo "Installing tmux with Homebrew..."
        brew install tmux
    elif [[ "$(uname)" =~ (CYGWIN|MINGW|MSYS) ]]; then
        echo "Windows (Git Bash/MSYS) environment detected."
        echo "Attempting to install tmux using pacman..."
        pacman -S --noconfirm tmux
    elif [[ "$(uname)" == "Linux" ]]; then
        if command -v apt-get &> /dev/null; then
            echo "This script will attempt to install tmux and ncurses dev library using apt-get. Sudo password may be required."
        sudo apt-get update
        sudo apt-get install -y tmux libncurses-dev
        elif command -v yum &> /dev/null; then
            echo "This script will attempt to install tmux and ncurses dev library using yum. Sudo password may be required."
            sudo yum install -y tmux ncurses-devel
        else
            echo "Could not find apt-get or yum. Please install tmux manually."
            exit 1
        fi
    else
        echo "Unsupported OS. Please install tmux manually."
        exit 1
    fi
    echo "tmux installed successfully."
else
    echo "tmux is already installed."
fi

# --- 빌드 및 준비 (Sudo 불필요) ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

echo "Creating necessary directories..."
mkdir -p "$PROJECT_ROOT/build"
mkdir -p "$PROJECT_ROOT/src/lib"

echo "Building C code..."
gcc -Wall -Isrc/core -o "$PROJECT_ROOT/src/lib/ui" "$PROJECT_ROOT/src/core/ui.c" "$PROJECT_ROOT/src/core/parser.c" -lncurses
if [ $? -ne 0 ]; then
    echo "C code compilation failed."
    exit 1
fi

GITSCORPE_SRC_SH="$PROJECT_ROOT/src/gitscope.sh"
GITSCORPE_BUILD_SH="$PROJECT_ROOT/build/gitscope.sh"

echo "Creating executable script at $GITSCORPE_BUILD_SH..."
cp "$GITSCORPE_SRC_SH" "$GITSCORPE_BUILD_SH"
sed -i.bak 's|source "$(dirname "$0")/modules/utils.sh"|source "$(dirname "$0")/../src/modules/utils.sh"|g' "$GITSCORPE_BUILD_SH"
chmod +x "$GITSCORPE_BUILD_SH"

echo ""
echo "Build successful!"
echo ""
echo "To make 'gitscope' command available everywhere, please run the following command manually:"
echo "sudo ln -sf \"$GITSCORPE_BUILD_SH\" /usr/local/bin/gitscope"
echo ""