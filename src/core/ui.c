#include "ui.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_lines(WINDOW *win, char **lines, int num_lines, int top_line, int highlight, int win_height, int win_width) {
    werase(win);
    box(win, 0, 0);
    for (int i = 0; i < win_height - 2; ++i) {
        int current_line = top_line + i;
        if (current_line >= num_lines) {
            break;
        }
        if (current_line == highlight) {
            wattron(win, A_REVERSE);
        }
        mvwprintw(win, i + 1, 1, "%.*s", win_width - 2, lines[current_line]);
        if (current_line == highlight) {
            wattroff(win, A_REVERSE);
        }
    }
    wrefresh(win);
}

int start_ui(const char* git_log_filepath) {
    if (initscr() == NULL) {
        return 1;
    }
    clear();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int height, width;
    getmaxyx(stdscr, height, width);

    if (width < 10) {
        endwin();
        return 1;
    }

    refresh();

    WINDOW *left_win = newwin(height, width / 2, 0, 0);
    WINDOW *right_win = newwin(height, width / 2, 0, width / 2);
    box(right_win, 0, 0);
    mvwprintw(right_win, 1, 1, "Welcome");
    wrefresh(right_win);

    FILE *fp = fopen(git_log_filepath, "r");
    if (fp == NULL) {
        mvwprintw(left_win, 1, 1, "Failed to open git log file.");
        wrefresh(left_win);
        getch();
        endwin();
        return 1;
    }

    int capacity = 20;
    int num_lines = 0;
    char **lines = (char **)malloc(sizeof(char*) * capacity);
    char buffer[2048];

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (num_lines >= capacity) {
            capacity *= 2;
            lines = (char **)realloc(lines, sizeof(char*) * capacity);
        }
        buffer[strcspn(buffer, "\n")] = 0;
        lines[num_lines] = strdup(buffer);
        num_lines++;
    }
    fclose(fp);

    int highlight = 0;
    int top_line = 0;
    int ch;

    print_lines(left_win, lines, num_lines, top_line, highlight, height, width / 2);

    while ((ch = getch()) != 'q') {
        switch (ch) {
            case KEY_UP:
            case 'k':
                if (highlight > 0) {
                    highlight--;
                    if (highlight < top_line) {
                        top_line = highlight;
                    }
                }
                break;
            case KEY_DOWN:
            case 'j':
                if (highlight < num_lines - 1) {
                    highlight++;
                    if (highlight >= top_line + height - 2) {
                        top_line = highlight - (height - 3);
                    }
                }
                break;
            case 'h':
                 highlight -= (height - 2);
                 if (highlight < 0) highlight = 0;
                 top_line = highlight;
                 break;
            case 'l':
                 highlight += (height - 2);
                 if (highlight >= num_lines) highlight = num_lines - 1;
                 top_line = highlight - (height - 3);
                 if (top_line < 0) top_line = 0;
                 break;
        }
        print_lines(left_win, lines, num_lines, top_line, highlight, height, width / 2);
    }

    for (int i = 0; i < num_lines; ++i) {
        free(lines[i]);
    }
    free(lines);

    endwin();
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }
    return start_ui(argv[1]);
}
