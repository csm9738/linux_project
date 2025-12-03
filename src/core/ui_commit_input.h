#ifndef UI_COMMIT_INPUT_H
#define UI_COMMIT_INPUT_H

// Enum for return values of handle_commit_input
typedef enum {
    UI_STATE_NO_CHANGE,           // No significant state change, stay in current mode
    UI_STATE_CHANGE_EXIT_COMMIT,  // Exit commit UI and return to MAIN_SCREEN
    UI_STATE_CHANGE_COMMIT_PERFORM, // Commit action requested
    UI_STATE_MESSAGE_FOCUS,       // Focus shifted to message input field
    UI_STATE_BUTTON_FOCUS,          // Focus shifted to commit/cancel buttons
    UI_STATE_CHANGE_PARSE_COMMIT_TYPES // Commit types input needs to be parsed
} CommitInputResult;


// This function handles key input specifically for the commit UI (COMMIT_SCREEN).
// It navigates the menu, confirms type selection, and handles Commit/Cancel actions.
int handle_commit_input(int ch, int *highlight_item, int *commit_type_selected_idx, char *current_commit_message, int *message_cursor_pos, char* user_commit_types_input, int* commit_type_input_cursor_pos, int max_items_in_commit_ui, int num_commit_types_in_ui);

#endif // UI_COMMIT_INPUT_H