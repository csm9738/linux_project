#!/bin/bash

source "$(dirname "$0")/modules/utils.sh"

save_current_pwd() {
    mkdir -p "$PROJECT_ROOT/build"
    pwd > "$PROJECT_ROOT/build/last_run_dir.txt"
}

save_current_pwd

show_logo

if [ "$#" -eq 0 ]; then
    "$PROJECT_ROOT/src/lib/ui"
fi
