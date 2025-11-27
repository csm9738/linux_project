#include "ui.h"
#include "parser.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void start_ui(const char* git_log_filepath) {
    initscr();
    clear();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int height, width;
    getmaxyx(stdscr, height, width);

    if (width < 10) {
        endwin();
        printf("Terminal is too small.\n");
        return;
    }

    refresh();

    // 왼쪽 창 생성
    WINDOW *left_win = newwin(height, width / 2, 0, 0);
    box(left_win, 0, 0);
    
    // 오른쪽 창 생성
    WINDOW *right_win = newwin(height, width / 2, 0, width / 2);
    box(right_win, 0, 0);
    mvwprintw(right_win, 1, 1, "Welcome");
    wrefresh(right_win);

    // git log 파싱 및 왼쪽 창에 출력
    int num_commits = 0;
    Commit* commits = parse_git_log_file(git_log_filepath, &num_commits);

    if (commits == NULL) {
        mvwprintw(left_win, 1, 1, "Failed to parse git log or no commits found.");
    } else {
        int y = 1;
        for (int i = 0; i < num_commits && y < height - 1; ++i) {
            mvwprintw(left_win, y, 1, "%.*s", (width / 2) - 2, commits[i].subject);
            y++;
        }
        free_commits(commits); // 메모리 해제
    }
    wrefresh(left_win);

    getch();

    endwin();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <git_log_filepath>\n", argv[0]);
        return 1;
    }
    start_ui(argv[1]);
    return 0;
}
