#!/bin/bash

source "$(dirname "$0")/modules/utils.sh"

save_current_pwd() {
    mkdir -p "$PROJECT_ROOT/build"
    pwd > "$PROJECT_ROOT/build/last_run_dir.txt"
}

save_current_pwd

show_logo

if [ "$#" -eq 0 ]; then
    GIT_LOG_FILE="$PROJECT_ROOT/build/git_log.txt"
    git --no-pager log --graph --all --color=always --pretty=format:'%C(auto)%h -%d%s %Cgreen(%ar) %C(bold blue)<%an>%Creset' > "$GIT_LOG_FILE"
    
    "$PROJECT_ROOT/src/lib/ui" "$GIT_LOG_FILE"
fi
