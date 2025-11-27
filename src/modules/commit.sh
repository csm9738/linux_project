#!/bin/bash

echo "--- Preparing for commit ---"

git add .
echo "All changes have been staged."

echo "Select commit type:"
echo "  1) feat: A new feature"
echo "  2) fix: A bug fix"
echo "  3) docs: Documentation only changes"
echo "  4) refactor: A code change that neither fixes a bug nor adds a feature"
echo "  5) test: Adding missing tests or correcting existing tests"
read -p "Enter number (1-5): " type_num

case $type_num in
    1) type="feat";;
    2) type="fix";;
    3) type="docs";;
    4) type="refactor";;
    5) type="test";;
    *) echo "Invalid selection. Aborting commit."; exit 1;;
esac

read -p "Enter commit message: " message

COMMIT_MSG="$type: $message"
echo "Committing with message: '$COMMIT_MSG'"
git commit -m "$COMMIT_MSG"

echo "Commit successful."
