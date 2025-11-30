#!/bin/bash

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$MODULE_DIR")")"

CONFIG_FILE="$PROJECT_ROOT/config/gitscope.conf"
if [ -f "$CONFIG_FILE" ]; then
    source "$CONFIG_FILE"
fi

VERSION_FILE="$PROJECT_ROOT/config/version"

function show_logo() {
    if [ -f "$VERSION_FILE" ]; then
        CURRENT_VERSION=$(cat "$VERSION_FILE")
    else
        CURRENT_VERSION="Unknown"
    fi

    cat << EOF
    ____   _  ____                        |
  / ___(_)||_/ ___|  ___ ___  _ __   ___  |
 | |  _| | __\___ \ / __/ _ \| '_ \ / _ \ |
 | |_| | | |_ ___) | (_| (_) | |_) |  __/ |
  \____|_|\__|____/ \___\___/| .__/ \___| |
                             |_|          | Version ${CURRENT_VERSION}

EOF
}

