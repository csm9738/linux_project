#include "ui_internal.h"
#include "ui_commit_input.h" // For its own prototype and enum
#include <ncurses.h>
#include <string.h>
#include <stdlib.h> // For exit codes and such

int handle_commit_input(int ch, int *highlight_item, int *commit_type_selected_idx, char *current_commit_message, int *message_cursor_pos, char* user_commit_types_input, int* commit_type_input_cursor_pos, int max_items_in_commit_ui, int num_commit_types_in_ui) {
    switch (ch) {
        case KEY_UP: case 'k':
            if (*highlight_item > 0) {
                (*highlight_item)--;
            }
            return UI_STATE_NO_CHANGE;
        case KEY_DOWN: case 'j':
            // max_items_in_commit_ui is now num_commit_types_in_ui + 4 (input_field + types + message + commit + cancel)
            if (*highlight_item < max_items_in_commit_ui - 1) { 
                (*highlight_item)++;
            }
            return UI_STATE_NO_CHANGE;
        case 27: // ESC key
            return UI_STATE_CHANGE_EXIT_COMMIT;
        case '\n': // Enter key
            if (*highlight_item == 0) { // Commit types input field
                // Trigger parsing of commit types
                return UI_STATE_CHANGE_PARSE_COMMIT_TYPES;
            } else if (*highlight_item >= 1 && *highlight_item <= num_commit_types_in_ui) { // Commit type selected (index 1 to num_commit_types_in_ui)
                *commit_type_selected_idx = *highlight_item - 1; // Adjust index because item 0 is input field
                *highlight_item = num_commit_types_in_ui + 1; // Move highlight to message field
                return UI_STATE_MESSAGE_FOCUS;
            } else if (*highlight_item == num_commit_types_in_ui + 1) { // Message field
                // If Enter is pressed on message field, just move to Commit button
                *highlight_item = num_commit_types_in_ui + 2; // Move to Commit button
                return UI_STATE_BUTTON_FOCUS;
            } else if (*highlight_item == num_commit_types_in_ui + 2) { // Commit button
                // This signals that commit should be performed
                return UI_STATE_CHANGE_COMMIT_PERFORM;
            } else if (*highlight_item == num_commit_types_in_ui + 3) { // Cancel button
                return UI_STATE_CHANGE_EXIT_COMMIT;
            }
            return UI_STATE_NO_CHANGE;
        default:
            return UI_STATE_NO_CHANGE;
    }
}
