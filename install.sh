#!/bin/bash

if ! command -v tmux &> /dev/null; then
    echo "tmux is not installed. Attempting to install..."
    
    if [[ "$(uname)" == "Darwin" ]]; then
        if ! command -v brew &> /dev/null; then
            echo "Homebrew is not installed. Please install it first."
            exit 1
        fi
        echo "Installing tmux with Homebrew..."
        brew install tmux
    elif [[ "$(uname)" == "Linux" ]]; then
        if command -v apt-get &> /dev/null; then
            sudo apt-get update && sudo apt-get install -y tmux libncurses-dev
        elif command -v yum &> /dev/null; then
            sudo yum install -y tmux ncurses-devel
        else
            echo "Could not find apt-get or yum. Please install tmux and ncurses-devel manually."
            exit 1
        fi
    else
        echo "Unsupported OS. Please install tmux manually."
        exit 1
    fi
else
    echo "tmux is already installed."
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build"
SRC_DIR="$PROJECT_ROOT/src"

echo "Creating necessary directories..."
mkdir -p "$BUILD_DIR/modules"
mkdir -p "$SRC_DIR/core"
mkdir -p "$SRC_DIR/lib"

echo "Building C code..."
make
# gcc -Wall -I"$SRC_DIR/core" -o "$BUILD_DIR/ui" "$SRC_DIR/core/ui.c" -lncurses
if [ $? -ne 0 ]; then
    echo "C code compilation failed."
    exit 1
fi

echo "Copying scripts to build directory..."
cp "$SRC_DIR/gitscope.sh" "$BUILD_DIR/gitscope.sh"
rm -rf "$BUILD_DIR/modules"
cp -r "$SRC_DIR/modules" "$BUILD_DIR/"

echo "Setting permissions for scripts..."
chmod +x "$BUILD_DIR/gitscope.sh"
chmod +x "$BUILD_DIR/modules/"*.sh

INSTALL_PATH="/usr/local/bin/gitscope"
TARGET_SCRIPT_PATH="$BUILD_DIR/gitscope.sh"

echo "Creating symlink to $INSTALL_PATH..."
if [ -L "$INSTALL_PATH" ] || [ -f "$INSTALL_PATH" ]; then
    sudo rm "$INSTALL_PATH"
fi
sudo ln -s "$TARGET_SCRIPT_PATH" "$INSTALL_PATH"

echo "Installation complete! You can now run 'gitscope' from anywhere."