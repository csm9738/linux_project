#include "ui_internal.h"
#include "parser.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <stdbool.h>

int start_ui(const char* git_log_filepath, const char* project_root) {
    setlocale(LC_ALL, "");
    initscr();
    if (has_colors() == FALSE) { endwin(); printf("Your terminal does not support color\n"); return 1; }
    start_color(); use_default_colors(); init_pair(11, COLOR_BLACK, COLOR_WHITE);
    init_pair(1, COLOR_WHITE, -1); init_pair(2, COLOR_RED, -1); init_pair(3, COLOR_GREEN, -1); init_pair(4, COLOR_YELLOW, -1); init_pair(5, COLOR_BLUE, -1); init_pair(6, COLOR_MAGENTA, -1); init_pair(7, COLOR_CYAN, -1); init_pair(8, COLOR_WHITE, -1);
    const char *env_border_color = getenv("HIGHLIGHT_COLOR");
    char current_border_color[16] = "cyan";
    if (env_border_color != NULL) { strncpy(current_border_color, env_border_color, sizeof(current_border_color) - 1); current_border_color[sizeof(current_border_color) - 1] = '\0'; }
    const char *env_tree_color = getenv("TREE_COLOR");
    char current_tree_color[16] = "green";
    if (env_tree_color != NULL) { strncpy(current_tree_color, env_tree_color, sizeof(current_tree_color) - 1); current_tree_color[sizeof(current_tree_color) - 1] = '\0'; }
    int border_col = COLOR_CYAN;
    if (strcmp(current_border_color, "blue") == 0) border_col = COLOR_BLUE; else if (strcmp(current_border_color, "green") == 0) border_col = COLOR_GREEN; else if (strcmp(current_border_color, "red") == 0) border_col = COLOR_RED; else if (strcmp(current_border_color, "magenta") == 0) border_col = COLOR_MAGENTA; else if (strcmp(current_border_color, "yellow") == 0) border_col = COLOR_YELLOW; else border_col = COLOR_CYAN;
    init_pair(12, border_col, -1);
    int tree_col = COLOR_GREEN;
    if (strcmp(current_tree_color, "blue") == 0) tree_col = COLOR_BLUE; else if (strcmp(current_tree_color, "green") == 0) tree_col = COLOR_GREEN; else if (strcmp(current_tree_color, "red") == 0) tree_col = COLOR_RED; else if (strcmp(current_tree_color, "magenta") == 0) tree_col = COLOR_MAGENTA; else if (strcmp(current_tree_color, "yellow") == 0) tree_col = COLOR_YELLOW; else tree_col = COLOR_GREEN;
    init_pair(9, tree_col, -1);
    const char *env_branch_palette = getenv("BRANCH_PALETTE");
    char current_branch_palette[16] = "single";
    if (env_branch_palette != NULL) { strncpy(current_branch_palette, env_branch_palette, sizeof(current_branch_palette) - 1); current_branch_palette[sizeof(current_branch_palette) - 1] = '\0'; }
    load_branch_palette_from_env();
    clear(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    refresh();
    int height, width; getmaxyx(stdscr, height, width);
    int divider_x = width / 2; bool resizing = false; MEVENT mevent;
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL); mouseinterval(0);
    printf("\033[?1002h\033[?1006h"); fflush(stdout);
    WINDOW *left_win = newwin(height, divider_x, 0, 0);
    WINDOW *right_win = newwin(height, width - divider_x, 0, divider_x);
    int num_lines = 0;
    LogLine *loglines = parse_log_file(git_log_filepath, &num_lines);
    if (loglines == NULL || num_lines == 0) { endwin(); return 1; }
    char **lines = (char **)malloc(sizeof(char*) * num_lines);
    for (int i = 0; i < num_lines; ++i) lines[i] = loglines[i].line_content;
    int top_line = 0, highlight_line = 0; int right_highlight = 0; int active_window = 0; int ch; int exit_code = 0; ScreenState current_screen = MAIN_SCREEN;
    char current_line_style[20] = "ascii";
    const char* env_line_style = getenv("LINE_STYLE");
    if (env_line_style != NULL) { strncpy(current_line_style, env_line_style, sizeof(current_line_style) - 1); current_line_style[sizeof(current_line_style) - 1] = '\0'; }
    while (1) {
        print_left_panel(left_win, lines, num_lines, top_line, (active_window == 0) ? highlight_line : -1, height, divider_x, current_line_style, current_tree_color, current_branch_palette);
        if (current_screen == MAIN_SCREEN) print_right_panel(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x);
        else if (current_screen == CUSTOMIZE_SCREEN) print_customize_menu(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x, current_line_style, current_border_color, current_tree_color, current_branch_palette);
        else if (current_screen == PALETTE_EDIT_SCREEN) print_palette_editor(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x);
        if (active_window == 0) { wattron(left_win, COLOR_PAIR(12) | A_BOLD); draw_border(left_win); wattroff(left_win, COLOR_PAIR(12) | A_BOLD); wattron(right_win, A_DIM); draw_border(right_win); wattroff(right_win, A_DIM); }
        else { wattron(right_win, COLOR_PAIR(12) | A_BOLD); draw_border(right_win); wattroff(right_win, COLOR_PAIR(12) | A_BOLD); wattron(left_win, A_DIM); draw_border(left_win); wattroff(left_win, A_DIM); }
        wnoutrefresh(left_win); wnoutrefresh(right_win); doupdate();
        if (resizing) timeout(20); else timeout(-1);
        ch = getch();
        switch (ch) {
            case KEY_MOUSE: {
                if (getmouse(&mevent) == OK) {
                    int mx = mevent.x; const int DRAG_THRESHOLD = 2;
                    if (mevent.bstate & BUTTON1_PRESSED) { if (abs(mx - divider_x) <= DRAG_THRESHOLD) resizing = true; }
                    if (resizing) {
                        int new_div = mx; if (new_div < 10) new_div = 10; if (new_div > width - 10) new_div = width - 10;
                        if (new_div != divider_x) {
                            divider_x = new_div; delwin(left_win); delwin(right_win);
                            left_win = newwin(height, divider_x, 0, 0); right_win = newwin(height, width - divider_x, 0, divider_x);
                            keypad(left_win, TRUE); keypad(right_win, TRUE); touchwin(left_win); touchwin(right_win);
                            print_left_panel(left_win, lines, num_lines, top_line, (active_window == 0) ? highlight_line : -1, height, divider_x, current_line_style, current_tree_color, current_branch_palette);
                            if (current_screen == MAIN_SCREEN) print_right_panel(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x);
                            else if (current_screen == CUSTOMIZE_SCREEN) print_customize_menu(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x, current_line_style, current_border_color, current_tree_color, current_branch_palette);
                            else if (current_screen == PALETTE_EDIT_SCREEN) print_palette_editor(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x);
                            wnoutrefresh(left_win); wnoutrefresh(right_win); doupdate();
                        }
                    }
                    if (mevent.bstate & BUTTON1_RELEASED) { resizing = false; timeout(-1); }
                }
            }
            break;
            case KEY_UP: case 'k':
                if (active_window == 0) {
                    if (highlight_line > 0) {
                        int search_start = highlight_line - 1;
                        int target = find_next_commit(lines, num_lines, search_start, -1);
                        if (target >= 0 && target != highlight_line) { highlight_line = target; if (highlight_line < top_line) top_line = highlight_line; }
                    }
                } else { if (current_screen != PALETTE_EDIT_SCREEN) { if (right_highlight > 0) right_highlight--; } }
                break;
            case KEY_DOWN: case 'j':
                if (active_window == 0) {
                    if (highlight_line < num_lines - 1) {
                        int search_start = highlight_line + 1;
                        int target = find_next_commit(lines, num_lines, search_start, 1);
                        if (target >= 0 && target != highlight_line) { highlight_line = target; if (highlight_line >= top_line + height - 2) top_line = highlight_line - (height - 3); }
                    }
                } else { if (current_screen != PALETTE_EDIT_SCREEN) { int max_menu = (current_screen == MAIN_SCREEN) ? 2 : 5; if (right_highlight < max_menu) right_highlight++; } }
                break;
            case KEY_LEFT: case 'h': active_window = 0; break;
            case KEY_RIGHT: case 'l': active_window = 1; break;
            case 'g':
                if (active_window == 0) {
                    int saved_to = resizing ? 20 : -1;
                    timeout(200);
                    int next = getch();
                    timeout(saved_to);
                    if (next == 'g') {
                        int target = find_next_commit(lines, num_lines, 0, 1);
                        if (target >= 0) { highlight_line = target; top_line = highlight_line; }
                    } else {
                        if (next != ERR) ungetch(next);
                    }
                }
                break;
            case 23:
                {
                    int saved_to = resizing ? 20 : -1;
                    timeout(200);
                    int nxt = getch();
                    timeout(saved_to);
                    if (nxt == 'w' || nxt == 'W' || nxt == 23) {
                        active_window = 1 - active_window;
                    } else {
                        if (nxt != ERR) ungetch(nxt);
                    }
                }
                break;
            case 'G':
                if (active_window == 0) {
                    int target = find_next_commit(lines, num_lines, num_lines - 1, -1);
                    if (target >= 0) { highlight_line = target; if (highlight_line >= top_line + height - 2) top_line = highlight_line - (height - 3); }
                }
                break;
            case '\n':
                if (active_window == 1) {
                    if (current_screen == MAIN_SCREEN) {
                        if (right_highlight == 0) { current_screen = CUSTOMIZE_SCREEN; right_highlight = 0; }
                        else if (right_highlight == 1) { exit_code = 2; goto end_loop; }
                        else if (right_highlight == 2) { exit_code = 3; goto end_loop; }
                    } else if (current_screen == CUSTOMIZE_SCREEN) {
                        if (right_highlight == 0) {
                            const char *styles[] = {"ascii","unicode","unicode-double","unicode-rounded"};
                            int ns = sizeof(styles) / sizeof(styles[0]); int idx = 0;
                            for (int si = 0; si < ns; ++si) if (strcmp(current_line_style, styles[si]) == 0) { idx = si; break; }
                            int next = (idx + 1) % ns; strncpy(current_line_style, styles[next], sizeof(current_line_style) - 1); current_line_style[sizeof(current_line_style) - 1] = '\0'; save_setting(project_root, "LINE_STYLE", current_line_style);
                        } else if (right_highlight == 1) {
                            const char *color_options[] = {"cyan","blue","green","red","magenta","yellow"}; int num_colors = sizeof(color_options) / sizeof(color_options[0]); int idx = 0;
                            for (int ci = 0; ci < num_colors; ++ci) if (strcmp(current_border_color, color_options[ci]) == 0) { idx = ci; break; }
                            int next = (idx + 1) % num_colors; strncpy(current_border_color, color_options[next], sizeof(current_border_color) - 1); current_border_color[sizeof(current_border_color) - 1] = '\0'; save_setting(project_root, "HIGHLIGHT_COLOR", current_border_color);
                            int border_col = COLOR_CYAN; if (strcmp(current_border_color, "blue") == 0) border_col = COLOR_BLUE; else if (strcmp(current_border_color, "green") == 0) border_col = COLOR_GREEN; else if (strcmp(current_border_color, "red") == 0) border_col = COLOR_RED; else if (strcmp(current_border_color, "magenta") == 0) border_col = COLOR_MAGENTA; else if (strcmp(current_border_color, "yellow") == 0) border_col = COLOR_YELLOW; else border_col = COLOR_CYAN; init_pair(12, border_col, -1);
                        } else if (right_highlight == 2) {
                            const char *color_options[] = {"cyan","blue","green","red","magenta","yellow"}; int num_colors = sizeof(color_options) / sizeof(color_options[0]); int idx = 0;
                            for (int ci = 0; ci < num_colors; ++ci) if (strcmp(current_tree_color, color_options[ci]) == 0) { idx = ci; break; }
                            int next = (idx + 1) % num_colors; strncpy(current_tree_color, color_options[next], sizeof(current_tree_color) - 1); current_tree_color[sizeof(current_tree_color) - 1] = '\0'; save_setting(project_root, "TREE_COLOR", current_tree_color);
                            int tree_col = COLOR_GREEN; if (strcmp(current_tree_color, "blue") == 0) tree_col = COLOR_BLUE; else if (strcmp(current_tree_color, "green") == 0) tree_col = COLOR_GREEN; else if (strcmp(current_tree_color, "red") == 0) tree_col = COLOR_RED; else if (strcmp(current_tree_color, "magenta") == 0) tree_col = COLOR_MAGENTA; else if (strcmp(current_tree_color, "yellow") == 0) tree_col = COLOR_YELLOW; else tree_col = COLOR_GREEN; init_pair(9, tree_col, -1); update_branch_color_map(current_branch_palette, current_tree_color);
                        } else if (right_highlight == 3) {
                            const char *palette_options[] = {"single","rainbow","alternate"}; int np = sizeof(palette_options) / sizeof(palette_options[0]); int pidx = 0; for (int pi = 0; pi < np; ++pi) if (strcmp(current_branch_palette, palette_options[pi]) == 0) { pidx = pi; break; }
                            int pnext = (pidx + 1) % np; strncpy(current_branch_palette, palette_options[pnext], sizeof(current_branch_palette) - 1); current_branch_palette[sizeof(current_branch_palette) - 1] = '\0'; save_setting(project_root, "BRANCH_PALETTE", current_branch_palette); update_branch_color_map(current_branch_palette, current_tree_color);
                        } else if (right_highlight == 4) { current_screen = PALETTE_EDIT_SCREEN; right_highlight = 0; }
                        else if (right_highlight == 5) { current_screen = MAIN_SCREEN; right_highlight = 0; }
                    }
                }
                break;
            case 'q': exit_code = 0; goto end_loop;
            default: break;
        }
        if (current_screen == PALETTE_EDIT_SCREEN) {
            if (ch == KEY_UP || ch == 'k') { if (right_highlight > 0) right_highlight--; else right_highlight = branch_palette_len; }
            else if (ch == KEY_DOWN || ch == 'j') { if (right_highlight < branch_palette_len) right_highlight++; else right_highlight = 0; }
            else if (ch == '\n') {
                if (right_highlight == branch_palette_len) { current_screen = CUSTOMIZE_SCREEN; right_highlight = 4; save_branch_palette_to_config(project_root); }
                else { const char *color_options[] = {"cyan","blue","green","red","magenta","yellow","white","black"}; int num_colors = sizeof(color_options) / sizeof(color_options[0]); int idx = 0; for (int ci = 0; ci < num_colors; ++ci) if (strcmp(branch_palette_colors[right_highlight], color_options[ci]) == 0) { idx = ci; break; } int next = (idx + 1) % num_colors; strncpy(branch_palette_colors[right_highlight], color_options[next], sizeof(branch_palette_colors[right_highlight]) - 1); branch_palette_colors[right_highlight][sizeof(branch_palette_colors[right_highlight]) - 1] = '\0'; update_branch_color_map(current_branch_palette, current_tree_color); }
            } else if (ch == 'q') { current_screen = MAIN_SCREEN; right_highlight = 0; }
        }
    }
end_loop:
    printf("\033[?1002l\033[?1006l"); fflush(stdout);
    free_log_lines(loglines, num_lines);
    free(lines);
    endwin();
    return exit_code;
}
