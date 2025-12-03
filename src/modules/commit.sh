#!/bin/bash

# ANSI color codes
COLOR_RESET="\033[0m"
COLOR_CYAN="\033[0;36m"
COLOR_GREEN="\033[0;32m"
COLOR_RED="\033[0;31m"
COLOR_YELLOW="\033[0;33m"

# Function to perform the git commit
perform_commit() {
    local commit_type=$1
    local commit_message=$2

    if [ -z "$commit_type" ] || [ -z "$commit_message" ]; then
        echo "${COLOR_RED}Error: Commit type or message is empty. Aborting commit.${COLOR_RESET}"
        exit 1
    fi

    local COMMIT_MSG="$commit_type: $commit_message"
    echo "${COLOR_CYAN}Committing with message: '${COMMIT_MSG}'${COLOR_RESET}"
    git commit -m "$COMMIT_MSG"

    if [ $? -eq 0 ]; then
        echo "${COLOR_GREEN}Commit successful.${COLOR_RESET}"
    else
        echo "${COLOR_RED}Error: Git commit failed. Aborting.${COLOR_RESET}"
        exit 1
    fi
}

# --- Main script logic ---

echo "${COLOR_CYAN}--- Preparing for commit ---${COLOR_RESET}"

git add .
if [ $? -ne 0 ]; then
    echo "${COLOR_RED}Error: git add . failed. Aborting commit.${COLOR_RESET}"
    exit 1
fi
echo "${COLOR_GREEN}All changes have been staged.${COLOR_RESET}"

COMMIT_TYPE_ARG=""
COMMIT_MESSAGE_ARG=""

# Check if commit type and message are provided as arguments
if [ "$#" -ge 2 ]; then
    # Validate commit type argument
    case "$1" in
        "feat"|"fix"|"docs"|"refactor"|"test")
            COMMIT_TYPE_ARG="$1"
            COMMIT_MESSAGE_ARG="$2"
            ;;
        *)
            echo "${COLOR_YELLOW}Warning: Invalid commit type argument '$1'. Falling back to interactive mode.${COLOR_RESET}"
            ;;
    esac
fi

# If arguments are valid, perform commit directly
if [ -n "$COMMIT_TYPE_ARG" ] && [ -n "$COMMIT_MESSAGE_ARG" ]; then
    perform_commit "$COMMIT_TYPE_ARG" "$COMMIT_MESSAGE_ARG"
    exit 0
fi

# Fallback to interactive mode if no valid arguments or insufficient arguments

read -p "$(echo -e "${COLOR_YELLOW}Enter desired commit types (comma-separated, e.g., feat,fix): ${COLOR_RESET}")" user_types_input

# Basic validation for user_types_input
if [ -z "$user_types_input" ]; then
    echo "${COLOR_RED}Error: No commit types entered. Aborting commit.${COLOR_RESET}"
    exit 1
fi

# Display user-specified types and prompt for final selection
echo "${COLOR_CYAN}Select one of your specified commit types:${COLOR_RESET}"
select selected_type in $(echo "$user_types_input" | tr ',' ' '); do
    if [ -n "$selected_type" ]; then
        commit_type="$selected_type"
        break
    else
        echo "${COLOR_RED}Invalid selection. Please choose from your provided types.${COLOR_RESET}"
    fi
done

read -p "$(echo -e "${COLOR_YELLOW}Enter commit message: ${COLOR_RESET}")" commit_message

perform_commit "$commit_type" "$commit_message"
