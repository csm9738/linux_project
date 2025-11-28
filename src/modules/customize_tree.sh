#!/bin/bash

CONFIG_DIR="$(dirname "$0")/../../config"
CONFIG_FILE="$CONFIG_DIR/gitscope.conf"

echo "--- Customize Tree Settings ---"

echo "Select Line Style:"
echo "  1) ASCII (e.g., |\-)"
echo "  2) Unicode (e.g., ─┬└)"
read -p "Enter number (1-2): " line_style_num

LINE_STYLE=""
if [ "$line_style_num" -eq 1 ]; then
    LINE_STYLE="ascii"
    echo "Line style set to ASCII"
elif [ "$line_style_num" -eq 2 ]; then
    LINE_STYLE="unicode"
    echo "Line style set to Unicode"
else
    echo "Invalid selection. Defaulting to ASCII."
    LINE_STYLE="ascii"
fi

mkdir -p "$CONFIG_DIR"
echo "LINE_STYLE=$LINE_STYLE" > "$CONFIG_FILE"
echo "Settings saved to $CONFIG_FILE"

read -p "Press Enter to return to GitScope..."
exit 0
