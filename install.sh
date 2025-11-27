#!/bin/bash

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
    elif [[ "$(uname)" == "Linux" ]]; then
        if command -v apt-get &> /dev/null; then
            echo "This script will attempt to install tmux using apt-get. Sudo password may be required."
            sudo apt-get update
            sudo apt-get install -y tmux
        elif command -v yum &> /dev/null; then
            echo "This script will attempt to install tmux using yum. Sudo password may be required."
            sudo yum install -y tmux
        else
            echo "Could not find apt-get or yum. Please install tmux manually."
            exit 1
        fi
    else
        echo "Unsupported OS. Please install tmux manually."
        exit 1
    fi

    if ! command -v tmux &> /dev/null; then
        echo "tmux installation failed. Please install it manually."
        exit 1
    fi
    echo "tmux installed successfully."
else
    echo "tmux is already installed."
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
GITSCORPE_SH="$PROJECT_ROOT/src/gitscope.sh"

echo "Making gitscope.sh executable..."
chmod +x "$GITSCORPE_SH"

INSTALL_PATH="/usr/local/bin/gitscope"

echo "Creating symlink to $INSTALL_PATH..."
if [ -L "$INSTALL_PATH" ]; then
    echo "Symlink already exists. Removing old one."
    sudo rm "$INSTALL_PATH"
elif [ -f "$INSTALL_PATH" ]; then
    echo "A file already exists at $INSTALL_PATH. Please remove it first."
    exit 1
fi

sudo ln -s "$GITSCORPE_SH" "$INSTALL_PATH"

echo "Installation complete! You can now run 'gitscope' from anywhere."