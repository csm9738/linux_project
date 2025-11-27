#include "ui.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int start_ui(const char* git_log_filepath) {
    if (initscr() == NULL) {
        return 1;
    }
    clear();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int height, width;
    getmaxyx(stdscr, height, width);

    if (width < 10) {
        endwin();
        return 1;
    }

    refresh();

    WINDOW *left_win = newwin(height, width / 2, 0, 0);
    box(left_win, 0, 0);
    
    WINDOW *right_win = newwin(height, width / 2, 0, width / 2);
    box(right_win, 0, 0);
    mvwprintw(right_win, 1, 1, "Welcome");
    wrefresh(right_win);

    FILE *fp = fopen(git_log_filepath, "r");
    if (fp == NULL) {
        mvwprintw(left_win, 1, 1, "Failed to open git log file.");
    } else {
        char buffer[2048];
        int y = 1;
        while (fgets(buffer, sizeof(buffer), fp) != NULL && y < height - 1) {
            buffer[strcspn(buffer, "\n")] = 0;
            mvwprintw(left_win, y, 1, "%.*s", (width / 2) - 2, buffer);
            y++;
        }
        fclose(fp);
    }
    wrefresh(left_win);

    getch();

    endwin();
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }
    return start_ui(argv[1]);
}