#!/bin/bash

# Provides a function to preview files changed in a specific commit.
#
# Usage: gitscope_preview <commit_hash>
#
gitscope_preview() {
    local commit_hash="$1"

    # Check if commit_hash is provided
    if [ -z "$commit_hash" ]; then
        echo "Usage: gitscope_preview <commit_hash>" >&2
        return 1
    fi

    # Get the list of changed files and loop through them
    git diff-tree --no-commit-id --name-only -r "$commit_hash" | while read -r file; do
        # Print a clear separator and the file name
        echo "=================================================="
        echo "FILE: $file"
        echo "--------------------------------------------------"

        # Show the first 30 lines of the file's content at that commit
        git show "$commit_hash":"$file" | head -n 30
        
        # Add a newline for spacing
        echo
    done
}