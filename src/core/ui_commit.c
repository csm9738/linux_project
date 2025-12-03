#include "ui_internal.h"
#include <ncurses.h>
#include <string.h>
#include <stdio.h>

void print_commit_ui(WINDOW *win, int highlight_item, const char *current_message, char** dynamic_commit_types, int dynamic_num_commit_types, const char* commit_types_input_buffer) {
    werase(win);
    box(win, 0, 0);

    mvwprintw(win, 1, 2, "--- Enter Commit Details ---");

    // Input field for dynamic commit types (highlight_item 0)
    if (highlight_item == 0) {
        wattron(win, A_REVERSE);
    }
    // mvwprintw(win, 3, 3, "Types (comma-separated): %s", commit_types_input_buffer ? commit_types_input_buffer : "");
    if (highlight_item == 0) {
        wattroff(win, A_REVERSE);
    }

    // Dynamic commit types list (highlight_item 1 to dynamic_num_commit_types)
    // Offset by 1 for the new input field
    for (int i = 0; i < dynamic_num_commit_types; ++i) {
        if (i + 1 == highlight_item) { // +1 because item 0 is the input field
            wattron(win, A_REVERSE);
        }
        mvwprintw(win, 5 + i, 3, "%s", dynamic_commit_types[i]);
        if (i + 1 == highlight_item) {
            wattroff(win, A_REVERSE);
        }
    }

    // Commit Message field (highlight_item dynamic_num_commit_types + 1)
    // Offset by 1 for the new input field and +2 for spacing
    if (highlight_item == dynamic_num_commit_types + 1) {
        wattron(win, A_REVERSE);
    }
    mvwprintw(win, 5 + dynamic_num_commit_types + 2, 3, "Commit Message: %s", current_message ? current_message : "");
    if (highlight_item == dynamic_num_commit_types + 1) {
        wattroff(win, A_REVERSE);
    }

    // Commit and Cancel buttons (highlight_item dynamic_num_commit_types + 2 and +3)
    int commit_button_idx = dynamic_num_commit_types + 2;
    int cancel_button_idx = dynamic_num_commit_types + 3;

    if (highlight_item == commit_button_idx) {
        wattron(win, A_REVERSE);
    }
    mvwprintw(win, 5 + dynamic_num_commit_types + 4, 3, "%s", "Commit");
    if (highlight_item == commit_button_idx) {
        wattroff(win, A_REVERSE);
    }

    if (highlight_item == cancel_button_idx) {
        wattron(win, A_REVERSE);
    }
    mvwprintw(win, 5 + dynamic_num_commit_types + 5, 3, "%s", "Cancel");
    if (highlight_item == cancel_button_idx) {
        wattroff(win, A_REVERSE);
    }

    wrefresh(win);
}