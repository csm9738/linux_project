#ifndef UI_COMMIT_H
#define UI_COMMIT_H

#include <ncurses.h>

void print_commit_ui(WINDOW *win, int highlight_item, const char *current_message);

#endif // UI_COMMIT_H