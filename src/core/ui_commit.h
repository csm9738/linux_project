#ifndef UI_COMMIT_H
#define UI_COMMIT_H

#include <ncurses.h>

void print_commit_ui(WINDOW *win, int highlight_item, const char *current_message, char** dynamic_commit_types, int dynamic_num_commit_types, const char* commit_types_input_buffer);

#endif // UI_COMMIT_H