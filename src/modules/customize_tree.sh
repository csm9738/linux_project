#!/bin/bash

CURRENT_STYLE=$(git config --global --get gitscope.style)
if [ -z "$CURRENT_STYLE" ]; then
    CURRENT_STYLE="ascii"
fi

echo "--- Customize Tree Style ---"
echo "Current style: $CURRENT_STYLE"
echo ""
echo "Select new style:"
echo "  1) ascii"
echo "  2) unicode (single line)"
echo "  3) unicode-double (double line)"
echo "  4) unicode-rounded (rounded corners)"
echo ""
read -p "Enter number (1-4): " SELECTION

case $SELECTION in
    1)
        git config --global gitscope.style "ascii"
        echo "Tree style set to ascii."
        ;;
    2)
        git config --global gitscope.style "unicode"
        echo "Tree style set to unicode (single line)."
        ;;
    3)
        git config --global gitscope.style "unicode-double"
        echo "Tree style set to unicode-double (double line)."
        ;;
    4)
        git config --global gitscope.style "unicode-rounded"
        echo "Tree style set to unicode-rounded (rounded corners)."
        ;;
    *)
        echo "Invalid selection. No changes made."
        ;;
esac