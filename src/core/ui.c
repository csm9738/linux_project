#include "ui.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void start_ui() {
    initscr();
    clear(); // 화면을 깨끗하게 시작
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int height, width;
    getmaxyx(stdscr, height, width);

    // 터미널 크기 확인
    if (width < 10) {
        endwin();
        printf("Terminal is too small.\n");
        return;
    }

    refresh(); // 메인 화면을 한번 그려줌

    // 왼쪽 창 생성
    WINDOW *left_win = newwin(height, width / 2, 0, 0);
    box(left_win, 0, 0);
    
    // 오른쪽 창 생성
    WINDOW *right_win = newwin(height, width / 2, 0, width / 2);
    box(right_win, 0, 0);
    mvwprintw(right_win, 1, 1, "Welcome");
    wrefresh(right_win);

    // 왼쪽 창에 git log 출력
    FILE *fp;
    char path[1035];
    
    fp = popen("git --no-pager log", "r");
    if (fp == NULL) {
        mvwprintw(left_win, 1, 1, "Failed to run git log command.");
    } else {
        int y = 1;
        while (fgets(path, sizeof(path), fp) != NULL && y < height - 1) {
            mvwprintw(left_win, y, 1, "%.*s", (width / 2) - 2, path);
            y++;
        }
        pclose(fp);
    }
    wrefresh(left_win);

    // 사용자 입력 대기
    getch();

    endwin();
}

int main() {
    start_ui();
    return 0;
}
