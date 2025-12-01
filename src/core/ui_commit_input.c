#include "ui_internal.h"
#include "ui_commit_input.h" // For its own prototype and enum
#include <ncurses.h>
#include <string.h>
#include <stdlib.h> // For exit codes and such

int handle_commit_input(int ch, int *highlight_item, int *commit_type_selected_idx, char *current_commit_message, int *message_cursor_pos, int max_items_in_commit_ui, int num_commit_types_in_ui) {
    switch (ch) {
        case KEY_UP: case 'k':
            if (*highlight_item > 0) {
                (*highlight_item)--;
            }
            return UI_STATE_NO_CHANGE;
        case KEY_DOWN: case 'j':
            if (*highlight_item < max_items_in_commit_ui - 1) { // max_items_in_commit_ui will be 8
                (*highlight_item)++;
            }
            return UI_STATE_NO_CHANGE;
        case 27: // ESC key
            return UI_STATE_CHANGE_EXIT_COMMIT;
        case '\n': // Enter key
            if (*highlight_item >= 0 && *highlight_item < num_commit_types_in_ui) { // Commit type selected (0-4)
                *commit_type_selected_idx = *highlight_item;
                *highlight_item = num_commit_types_in_ui; // Move highlight to message field (index 5)
                return UI_STATE_MESSAGE_FOCUS;
            } else if (*highlight_item == num_commit_types_in_ui) { // Message field (index 5)
                // If Enter is pressed on message field, just move to Commit button
                *highlight_item = num_commit_types_in_ui + 1; // Move to Commit button (index 6)
                return UI_STATE_BUTTON_FOCUS;
            } else if (*highlight_item == num_commit_types_in_ui + 1) { // Commit button (index 6)
                // This signals that commit should be performed
                return UI_STATE_CHANGE_COMMIT_PERFORM;
            } else if (*highlight_item == num_commit_types_in_ui + 2) { // Cancel button (index 7)
                return UI_STATE_CHANGE_EXIT_COMMIT;
            }
            return UI_STATE_NO_CHANGE;

        // General character input for message field (this will be handled in ui_main.c)
        // Backspace for message field (this will be handled in ui_main.c)
        default:
            return UI_STATE_NO_CHANGE;
    }
}
