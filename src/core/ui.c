#include "ui.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void show_popup(const char* content); // Forward declaration

void run_preview(const char* commit_line) {
    char hash[41]; // Git hash is typically 40 chars + null terminator
    
    // Find the start of the hash (skip potential graph characters like '|', '*', ' ', '/')
    const char* hash_start = commit_line;
    while (*hash_start != '\0' && (*hash_start == '*' || *hash_start == '|' || *hash_start == ' ' || *hash_start == '/')) {
        hash_start++;
    }

    if (sscanf(hash_start, "%40s", hash) != 1) {
        show_popup("Could not parse commit hash.");
        return;
    }

    char command[512];
    snprintf(command, sizeof(command), "./src/modules/preview.sh %s", hash);

    FILE* fp = popen(command, "r");
    if (fp == NULL) {
        show_popup("Failed to run preview script. Make sure it is executable.");
        return;
    }

    char buffer[16384]; // Increased buffer size for larger previews
    size_t n = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[n] = '\0';
    pclose(fp);

    if (n == 0) {
        show_popup("Preview script returned no output.");
        return;
    }

    show_popup(buffer);
}

void show_popup(const char* content) {
    int height, width;
    getmaxyx(stdscr, height, width);

    int p_height = height * 0.8;
    int p_width = width * 0.8;
    int p_y = (height - p_height) / 2;
    int p_x = (width - p_width) / 2;

    WINDOW *popup_win = newwin(p_height, p_width, p_y, p_x);
    box(popup_win, 0, 0);
    
    // Print content line by line
    int max_lines = p_height - 2;
    int max_cols = p_width - 2;
    char *dup_content = strdup(content);
    char *line = strtok(dup_content, "\n");
    int line_num = 0;
    while(line != NULL && line_num < max_lines) {
        mvwprintw(popup_win, line_num + 1, 1, "%.*s", max_cols, line);
        line = strtok(NULL, "\n");
        line_num++;
    }
    free(dup_content);

    wrefresh(popup_win);
    
    // Wait for any key to close
    getch();

    delwin(popup_win);
}

void draw_border(WINDOW *win, int color_pair) {
    wattron(win, COLOR_PAIR(color_pair));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(color_pair));
}

void print_left_panel(WINDOW *win, char **lines, int num_lines, int top_line, int highlight_line, int win_height, int win_width) {
    werase(win);
    for (int i = 0; i < win_height - 2; ++i) {
        int current_line = top_line + i;
        if (current_line >= num_lines) break;
        if (current_line == highlight_line) wattron(win, A_REVERSE);
        mvwprintw(win, i + 1, 1, "%.*s", win_width - 2, lines[current_line]);
        if (current_line == highlight_line) wattroff(win, A_REVERSE);
    }
}

void print_right_panel(WINDOW *win, const char** menu_items, int num_items, int highlight_item) {
    werase(win);
    mvwprintw(win, 1, 1, "Welcome");
    mvwprintw(win, 3, 1, "Press 'q' to quit");
    
    for (int i = 0; i < num_items; ++i) {
        if (i == highlight_item) wattron(win, A_REVERSE);
        mvwprintw(win, 5 + i, 1, "%s", menu_items[i]);
        if (i == highlight_item) wattroff(win, A_REVERSE);
    }
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
    init_pair(1, COLOR_WHITE, -1);
    init_pair(2, COLOR_CYAN, -1); // 하늘색으로 변경

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

    const char *right_menu_items[] = {"Preview", "Customize Tree", "Run Tests", "Commit"};
    int num_right_menu_items = sizeof(right_menu_items) / sizeof(char*);

    int left_highlight = 0;
    int top_line = 0;
    int right_highlight = 0;
    int active_window = 0;
    int ch;
    int exit_code = 0;

    while (1) {
        print_left_panel(left_win, lines, num_lines, top_line, (active_window == 0) ? left_highlight : -1, height, width / 2);
        print_right_panel(right_win, right_menu_items, num_right_menu_items, (active_window == 1) ? right_highlight : -1);
        
        draw_border(left_win, (active_window == 0) ? 2 : 1);
        draw_border(right_win, (active_window == 1) ? 2 : 1);
        
        wnoutrefresh(left_win);
        wnoutrefresh(right_win);
        doupdate();

        ch = getch();
        if (ch == 'q') break;

        switch (ch) {
            case KEY_LEFT: case 'h':
                active_window = 0;
                break;
            case KEY_RIGHT: case 'l':
                active_window = 1;
                break;
            case '\n':
                if (active_window == 1) { // Right panel is active
                    if (right_highlight == 0) { // "Preview" selected
                        run_preview(lines[left_highlight]);
                    } else if (right_highlight == 1) { // "Customize Tree" selected
                        // This might have a different exit code or action later
                    } else if (right_highlight == 2) { // "Run Tests" selected
                        exit_code = 2;
                        goto end_loop;
                    } else if (right_highlight == 3) { // "Commit" selected
                        exit_code = 3;
                        goto end_loop;
                    }
                }
                break;
        }



        if (active_window == 0) {
            switch (ch) {
                case KEY_UP: case 'k':
                    if (left_highlight > 0) {
                        left_highlight--;
                        if (left_highlight < top_line) top_line = left_highlight;
                    }
                    break;
                case KEY_DOWN: case 'j':
                    if (left_highlight < num_lines - 1) {
                        left_highlight++;
                        if (left_highlight >= top_line + height - 2) top_line = left_highlight - (height - 3);
                    }
                    break;
            }
        } else { // active_window == 1
            switch (ch) {
                case KEY_UP: case 'k':
                    if (right_highlight > 0) right_highlight--;
                    break;
                case KEY_DOWN: case 'j':
                    if (right_highlight < num_right_menu_items - 1) right_highlight++;
                    break;
            }
        }


    }

end_loop:
    for (int i = 0; i < num_lines; ++i) free(lines[i]);
    free(lines);
    endwin();
    return exit_code;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    return start_ui(argv[1]);
}
