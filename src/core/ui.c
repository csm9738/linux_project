#include "ui.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void draw_border(WINDOW *win, int color_pair) {
    wattron(win, COLOR_PAIR(color_pair));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(color_pair));
}

void print_lines(WINDOW *win, char **lines, int num_lines, int top_line, int highlight, int win_height, int win_width) {
    werase(win);
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
    if (initscr() == NULL) return 1;
    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color\n");
        return 1;
    }

    start_color();
    use_default_colors();
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_BLUE, -1);
    init_pair(5, COLOR_MAGENTA, -1);
    init_pair(6, COLOR_CYAN, -1);
    init_pair(7, COLOR_WHITE, -1);
    init_pair(8, COLOR_GREEN, -1);

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
    int active_window = 0;
    int ch;

    while ((ch = getch()) != 'q') {
        if (active_window == 0) {
            switch (ch) {
                case KEY_UP:
                case 'k':
                    if (highlight > 0) {
                        highlight--;
                        if (highlight < top_line) top_line = highlight;
                    }
                    break;
                case KEY_DOWN:
                case 'j':
                    if (highlight < num_lines - 1) {
                        highlight++;
                        if (highlight >= top_line + height - 2) top_line = highlight - (height - 3);
                    }
                    break;
            }
        }
        
        switch (ch) {
            case KEY_RIGHT:
            case 'l':
                active_window = 1;
                break;
            case KEY_LEFT:
            case 'h':
                active_window = 0;
                break;
        }

        draw_border(left_win, (active_window == 0) ? 8 : 7);
        draw_border(right_win, (active_window == 1) ? 8 : 7);
        print_lines(left_win, lines, num_lines, top_line, highlight, height, width / 2);
        wrefresh(right_win);
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