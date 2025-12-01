#include "ui_internal.h"
#include <ncurses.h>
#include <string.h>
#include <stdio.h>

void print_commit_ui(WINDOW *win, int highlight_item, const char *current_message) {
    werase(win);
    box(win, 0, 0);

    mvwprintw(win, 1, 2, "--- Select Commit Type ---");

    const char *commit_types[] = {
        "feat: A new feature",
        "fix: A bug fix",
        "docs: Documentation only changes",
        "refactor: A code change that neither fixes a bug nor adds a feature",
        "test: Adding missing tests or correcting existing tests"
    };
    int num_commit_types = sizeof(commit_types) / sizeof(char*);

    for (int i = 0; i < num_commit_types; ++i) {
        if (i == highlight_item) {
            wattron(win, A_REVERSE);
        }
        mvwprintw(win, 3 + i, 3, "%s", commit_types[i]);
        if (i == highlight_item) {
            wattroff(win, A_REVERSE);
        }
    }

    mvwprintw(win, 3 + num_commit_types + 2, 2, "Commit Message: %s", current_message ? current_message : "");

    // Add Commit and Cancel buttons
    mvwprintw(win, 3 + num_commit_types + 4, 3, "%s", "Commit");
    mvwprintw(win, 3 + num_commit_types + 5, 3, "%s", "Cancel");

    wrefresh(win);
}