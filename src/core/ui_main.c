#include "ui_internal.h"
#include "parser.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <stdbool.h>

static void build_header(char *buf, size_t sz, LogLine *ln) {
    if (!ln) { if (sz > 0) buf[0] = '\0'; return; }
    char sh[8] = "";
    if (ln->hash[0]) { strncpy(sh, ln->hash, 7); sh[7] = '\0'; }
    if (ln->author[0] && ln->date[0])
        snprintf(buf, sz, "%s %s — %s on %s", sh, ln->subject, ln->author, ln->date);
    else
        snprintf(buf, sz, "%s %s", sh, ln->subject);
}

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
    int default_right_top_h = height / 6;
    if (default_right_top_h < 6) default_right_top_h = 6;
    if (default_right_top_h > height - 6) default_right_top_h = height / 3;
    int right_top_h = default_right_top_h;
    WINDOW *right_top = newwin(right_top_h, width - divider_x, 0, divider_x);
    WINDOW *right_bottom = newwin(height - right_top_h, width - divider_x, right_top_h, divider_x);
    int num_lines = 0;
    LogLine *loglines = parse_log_file(git_log_filepath, &num_lines);
    if (loglines == NULL || num_lines == 0) { endwin(); return 1; }
    char **lines = (char **)malloc(sizeof(char*) * num_lines);
    for (int i = 0; i < num_lines; ++i) lines[i] = loglines[i].line_content;
    int top_line = 0, highlight_line = 0; int right_highlight = 0; int active_window = 0; int ch; int exit_code = 0; ScreenState current_screen = MAIN_SCREEN;
    char current_line_style[20] = "ascii";
    const char* env_line_style = getenv("LINE_STYLE");
    if (env_line_style != NULL) { strncpy(current_line_style, env_line_style, sizeof(current_line_style) - 1); current_line_style[sizeof(current_line_style) - 1] = '\0'; }
    int preview_top = 0;
    char **file_list = NULL; int file_list_count = 0; int preview_selected = 0;
    char **file_diff = NULL; int file_diff_count = 0;
    int preview_mode = 0;
    char header_buf[1024]; header_buf[0] = '\0';
    file_list = get_commit_changed_files(project_root, loglines[highlight_line].hash, &file_list_count);
    build_header(header_buf, sizeof(header_buf), &loglines[highlight_line]);
    while (1) {
        int right_w = width - divider_x;
        print_left_panel(left_win, lines, num_lines, top_line, (active_window == 0) ? highlight_line : -1, height, divider_x, current_line_style, current_tree_color, current_branch_palette);
        if (current_screen == MAIN_SCREEN) print_right_panel(right_top, (active_window == 1) ? right_highlight : -1, right_top_h, right_w);
        else if (current_screen == CUSTOMIZE_SCREEN) print_customize_menu(right_top, (active_window == 1) ? right_highlight : -1, right_top_h, right_w, current_line_style, current_border_color, current_tree_color, current_branch_palette);
        else if (current_screen == PALETTE_EDIT_SCREEN) print_palette_editor(right_top, (active_window == 1) ? right_highlight : -1, right_top_h, right_w);
        if (preview_mode == 0) print_preview(right_bottom, file_list, file_list_count, preview_top, height - right_top_h, right_w, header_buf, 1, preview_selected, project_root, loglines[highlight_line].hash);
        else print_preview(right_bottom, file_diff, file_diff_count, preview_top, height - right_top_h, right_w, header_buf, 0, -1, project_root, loglines[highlight_line].hash);
        if (active_window == 0) {
            wattron(left_win, COLOR_PAIR(12) | A_BOLD);
            draw_border(left_win);
            wattroff(left_win, COLOR_PAIR(12) | A_BOLD);
            wattron(right_top, A_DIM); draw_border(right_top); wattroff(right_top, A_DIM);
            wattron(right_bottom, A_DIM); draw_border(right_bottom); wattroff(right_bottom, A_DIM);
        } else if (active_window == 1) {
            wattron(right_top, COLOR_PAIR(12) | A_BOLD); draw_border(right_top); wattroff(right_top, COLOR_PAIR(12) | A_BOLD);
            wattron(left_win, A_DIM); draw_border(left_win); wattroff(left_win, A_DIM);
            wattron(right_bottom, A_DIM); draw_border(right_bottom); wattroff(right_bottom, A_DIM);
        } else {
            wattron(right_bottom, COLOR_PAIR(12) | A_BOLD); draw_border(right_bottom); wattroff(right_bottom, COLOR_PAIR(12) | A_BOLD);
            wattron(left_win, A_DIM); draw_border(left_win); wattroff(left_win, A_DIM);
            wattron(right_top, A_DIM); draw_border(right_top); wattroff(right_top, A_DIM);
        }
        wnoutrefresh(left_win); wnoutrefresh(right_top); wnoutrefresh(right_bottom); doupdate();
        if (resizing) timeout(20); else timeout(-1);
        ch = getch();
        switch (ch) {
            case KEY_RESIZE: {
                int new_h, new_w;
                endwin();
                refresh();
                clear();
                getmaxyx(stdscr, new_h, new_w);
                height = new_h; width = new_w;
                /* clamp divider and right-top height to sensible ranges */
                if (divider_x > width - 10) divider_x = width / 2;
                if (divider_x < 10) divider_x = 10;
                if (right_top_h > height - 6) right_top_h = height - 6;
                if (right_top_h < 6) right_top_h = 6;
                default_right_top_h = height / 6;
                if (default_right_top_h < 6) default_right_top_h = 6;
                if (default_right_top_h > height - 6) default_right_top_h = height / 3;
                delwin(left_win); delwin(right_top); delwin(right_bottom);
                left_win = newwin(height, divider_x, 0, 0);
                right_top = newwin(right_top_h, width - divider_x, 0, divider_x);
                right_bottom = newwin(height - right_top_h, width - divider_x, right_top_h, divider_x);
                keypad(left_win, TRUE); keypad(right_top, TRUE); keypad(right_bottom, TRUE);
                touchwin(left_win); touchwin(right_top); touchwin(right_bottom);
                wnoutrefresh(left_win); wnoutrefresh(right_top); wnoutrefresh(right_bottom); doupdate();
            }
            break;
            case KEY_MOUSE: {
                if (getmouse(&mevent) == OK) {
                    int mx = mevent.x; const int DRAG_THRESHOLD = 2;
                    if (mevent.bstate & BUTTON1_PRESSED) { if (abs(mx - divider_x) <= DRAG_THRESHOLD) resizing = true; }
                    if (resizing) {
                        int new_div = mx; if (new_div < 10) new_div = 10; if (new_div > width - 10) new_div = width - 10;
                        if (new_div != divider_x) {
                            divider_x = new_div; delwin(left_win); delwin(right_top); delwin(right_bottom);
                            left_win = newwin(height, divider_x, 0, 0);
                            right_top = newwin(right_top_h, width - divider_x, 0, divider_x);
                            right_bottom = newwin(height - right_top_h, width - divider_x, right_top_h, divider_x);
                            keypad(left_win, TRUE); keypad(right_top, TRUE); keypad(right_bottom, TRUE); touchwin(left_win); touchwin(right_top); touchwin(right_bottom);
                            print_left_panel(left_win, lines, num_lines, top_line, (active_window == 0) ? highlight_line : -1, height, divider_x, current_line_style, current_tree_color, current_branch_palette);
                            if (current_screen == MAIN_SCREEN) print_right_panel(right_top, (active_window == 1) ? right_highlight : -1, right_top_h, width - divider_x);
                            else if (current_screen == CUSTOMIZE_SCREEN) print_customize_menu(right_top, (active_window == 1) ? right_highlight : -1, right_top_h, width - divider_x, current_line_style, current_border_color, current_tree_color, current_branch_palette);
                            else if (current_screen == PALETTE_EDIT_SCREEN) print_palette_editor(right_top, (active_window == 1) ? right_highlight : -1, right_top_h, width - divider_x);
                            if (preview_mode == 0) print_preview(right_bottom, file_list, file_list_count, preview_top, height - right_top_h, width - divider_x, header_buf, 1, preview_selected, project_root, loglines[highlight_line].hash);
                            else print_preview(right_bottom, file_diff, file_diff_count, preview_top, height - right_top_h, width - divider_x, header_buf, 0, -1, project_root, loglines[highlight_line].hash);
                            wnoutrefresh(left_win); wnoutrefresh(right_top); wnoutrefresh(right_bottom); doupdate();
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
                        if (file_list) free_string_lines(file_list, file_list_count);
                        file_list = get_commit_changed_files(project_root, loglines[highlight_line].hash, &file_list_count);
                        preview_selected = 0; preview_top = 0; preview_mode = 0;
                        if (file_diff) { free_string_lines(file_diff, file_diff_count); file_diff = NULL; file_diff_count = 0; }
                        build_header(header_buf, sizeof(header_buf), &loglines[highlight_line]);
                    }
                } else if (active_window == 1) { if (current_screen != PALETTE_EDIT_SCREEN) { if (right_highlight > 0) right_highlight--; } } else if (active_window == 2) {
                    if (preview_mode == 0) {
                        if (preview_selected > 0) { preview_selected--; if (preview_selected < preview_top) preview_top = preview_selected; }
                    } else {
                        if (preview_top > 0) preview_top--;
                    }
                }
                break;
            case KEY_DOWN: case 'j':
                if (active_window == 0) {
                    if (highlight_line < num_lines - 1) {
                        int search_start = highlight_line + 1;
                        int target = find_next_commit(lines, num_lines, search_start, 1);
                        if (target >= 0 && target != highlight_line) { highlight_line = target; if (highlight_line >= top_line + height - 2) top_line = highlight_line - (height - 3); }
                        if (file_list) free_string_lines(file_list, file_list_count);
                        file_list = get_commit_changed_files(project_root, loglines[highlight_line].hash, &file_list_count);
                        preview_selected = 0; preview_top = 0; preview_mode = 0;
                        if (file_diff) { free_string_lines(file_diff, file_diff_count); file_diff = NULL; file_diff_count = 0; }
                        build_header(header_buf, sizeof(header_buf), &loglines[highlight_line]);
                    }
                } else if (active_window == 1) { if (current_screen != PALETTE_EDIT_SCREEN) { int max_menu = (current_screen == MAIN_SCREEN) ? 1 : 5; if (right_highlight < max_menu) right_highlight++; } } else if (active_window == 2) {
                    if (preview_mode == 0) {
                        if (preview_selected < file_list_count - 1) { preview_selected++; int content_h = height - right_top_h - 2; if (preview_selected >= preview_top + content_h) preview_top = preview_selected - content_h + 1; }
                    } else {
                        int content_h = height - right_top_h - 2; if (preview_top + content_h < file_diff_count) preview_top++;
                    }
                }
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
                        if (target >= 0) {
                            highlight_line = target; top_line = highlight_line;
                            if (file_list) free_string_lines(file_list, file_list_count);
                            file_list = get_commit_changed_files(project_root, loglines[highlight_line].hash, &file_list_count);
                            preview_selected = 0; preview_top = 0; preview_mode = 0;
                            if (file_diff) { free_string_lines(file_diff, file_diff_count); file_diff = NULL; file_diff_count = 0; }
                            build_header(header_buf, sizeof(header_buf), &loglines[highlight_line]);
                        }
                    } else {
                        if (next != ERR) ungetch(next);
                    }
                } else if (active_window == 2) {
                    int saved_to = resizing ? 20 : -1; timeout(200); int next = getch(); timeout(saved_to);
                    if (next == 'g') {
                        if (preview_mode == 0) { preview_selected = 0; preview_top = 0; }
                        else { preview_top = 0; }
                    } else { if (next != ERR) ungetch(next); }
                }
                break;
                case 23:
                {
                    int saved_to = resizing ? 20 : -1;
                    timeout(200);
                    int nxt = getch();
                    timeout(saved_to);
                    if (nxt == 'w' || nxt == 'W' || nxt == 23) {
                        active_window = (active_window + 1) % 3;
                    } else {
                        if (nxt != ERR) ungetch(nxt);
                    }
                }
                break;
            case 'G':
                if (active_window == 0) {
                    int target = find_next_commit(lines, num_lines, num_lines - 1, -1);
                    if (target >= 0) { highlight_line = target; if (highlight_line >= top_line + height - 2) top_line = highlight_line - (height - 3); }
                } else if (active_window == 2) {
                    
                    if (preview_mode == 0) {
                        preview_selected = file_list_count > 0 ? file_list_count - 1 : 0;
                        int content_h = height - right_top_h - 2;
                        if (preview_selected >= content_h) preview_top = preview_selected - content_h + 1; else preview_top = 0;
                    } else {
                        int content_h = height - right_top_h - 2;
                        if (file_diff_count > 0) preview_top = (file_diff_count > content_h) ? file_diff_count - content_h : 0; else preview_top = 0;
                    }
                }
                break;
            
            case '\n':
                if (active_window == 0) {
                    
                    int commit_idx = highlight_line;
                    if (!loglines[commit_idx].is_commit) {
                        int prev = find_next_commit(lines, num_lines, commit_idx, -1);
                        if (prev >= 0) commit_idx = prev;
                        else {
                            int nxt = find_next_commit(lines, num_lines, commit_idx, 1);
                            if (nxt >= 0) commit_idx = nxt;
                        }
                    }
                    if (commit_idx >= 0 && loglines[commit_idx].hash[0]) {
                        highlight_line = commit_idx;
                        if (file_list) free_string_lines(file_list, file_list_count);
                        file_list = get_commit_changed_files(project_root, loglines[commit_idx].hash, &file_list_count);
                        if (file_diff) { free_string_lines(file_diff, file_diff_count); file_diff = NULL; file_diff_count = 0; }
                        preview_mode = 0; preview_selected = 0; preview_top = 0;
                        build_header(header_buf, sizeof(header_buf), &loglines[commit_idx]);
                        active_window = 2;
                    }
                } else if (active_window == 1) {
                        if (current_screen == MAIN_SCREEN) {
                        if (right_highlight == 0) {
                            current_screen = CUSTOMIZE_SCREEN; right_highlight = 0;
                            int new_h = height / 2;
                            if (new_h > height - 6) new_h = height - 6;
                            if (new_h < 6) new_h = 6;
                            if (new_h != right_top_h) {
                                right_top_h = new_h;
                                delwin(right_top); delwin(right_bottom);
                                right_top = newwin(right_top_h, width - divider_x, 0, divider_x);
                                right_bottom = newwin(height - right_top_h, width - divider_x, right_top_h, divider_x);
                                keypad(left_win, TRUE); keypad(right_top, TRUE); keypad(right_bottom, TRUE);
                                touchwin(left_win); touchwin(right_top); touchwin(right_bottom);
                            }
                        }
                        else if (right_highlight == 1) { exit_code = 3; goto end_loop; }
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
                        else if (right_highlight == 5) {
                            current_screen = MAIN_SCREEN; right_highlight = 0;
                            if (right_top_h != default_right_top_h) {
                                right_top_h = default_right_top_h;
                                delwin(right_top); delwin(right_bottom);
                                right_top = newwin(right_top_h, width - divider_x, 0, divider_x);
                                right_bottom = newwin(height - right_top_h, width - divider_x, right_top_h, divider_x);
                                keypad(left_win, TRUE); keypad(right_top, TRUE); keypad(right_bottom, TRUE);
                                touchwin(left_win); touchwin(right_top); touchwin(right_bottom);
                            }
                        }
                    }
                } else if (active_window == 2) {
                    /* preview Enter: if we're on file list, open selected file diff; if already showing file diff, go back to file list */
                    if (preview_mode == 0) {
                        if (preview_selected >= 0 && preview_selected < file_list_count) {
                            const char *path = file_list[preview_selected];
                            if (file_diff) { free_string_lines(file_diff, file_diff_count); file_diff = NULL; file_diff_count = 0; }
                            file_diff = get_commit_file_diff_lines(project_root, loglines[highlight_line].hash, path, &file_diff_count);
                            preview_mode = 1; preview_top = 0;
                        }
                    } else {
                        /* go back to file list: reset scroll to top but keep focus on the file we viewed */
                        if (file_diff) { free_string_lines(file_diff, file_diff_count); file_diff = NULL; file_diff_count = 0; }
                        preview_mode = 0;
                        /* ensure selected index is valid */
                        if (preview_selected < 0) preview_selected = 0;
                        if (preview_selected >= file_list_count) preview_selected = file_list_count > 0 ? file_list_count - 1 : 0;
                        preview_top = 0;
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
                if (right_highlight == branch_palette_len) {
                    current_screen = CUSTOMIZE_SCREEN; right_highlight = 4; save_branch_palette_to_config(project_root);
                    int new_h = height / 2;
                    if (new_h > height - 6) new_h = height - 6;
                    if (new_h < 6) new_h = 6;
                    if (new_h != right_top_h) {
                        right_top_h = new_h;
                        delwin(right_top); delwin(right_bottom);
                        right_top = newwin(right_top_h, width - divider_x, 0, divider_x);
                        right_bottom = newwin(height - right_top_h, width - divider_x, right_top_h, divider_x);
                        keypad(left_win, TRUE); keypad(right_top, TRUE); keypad(right_bottom, TRUE);
                        touchwin(left_win); touchwin(right_top); touchwin(right_bottom);
                    }
                }
                else { const char *color_options[] = {"cyan","blue","green","red","magenta","yellow","white","black"}; int num_colors = sizeof(color_options) / sizeof(color_options[0]); int idx = 0; for (int ci = 0; ci < num_colors; ++ci) if (strcmp(branch_palette_colors[right_highlight], color_options[ci]) == 0) { idx = ci; break; } int next = (idx + 1) % num_colors; strncpy(branch_palette_colors[right_highlight], color_options[next], sizeof(branch_palette_colors[right_highlight]) - 1); branch_palette_colors[right_highlight][sizeof(branch_palette_colors[right_highlight]) - 1] = '\0'; update_branch_color_map(current_branch_palette, current_tree_color); }
            } else if (ch == 'q') {
                current_screen = MAIN_SCREEN; right_highlight = 0;
                if (right_top_h != default_right_top_h) {
                    right_top_h = default_right_top_h;
                    delwin(right_top); delwin(right_bottom);
                    right_top = newwin(right_top_h, width - divider_x, 0, divider_x);
                    right_bottom = newwin(height - right_top_h, width - divider_x, right_top_h, divider_x);
                    keypad(left_win, TRUE); keypad(right_top, TRUE); keypad(right_bottom, TRUE);
                    touchwin(left_win); touchwin(right_top); touchwin(right_bottom);
                }
            }
        }
    }
end_loop:
    printf("\033[?1002l\033[?1006l"); fflush(stdout);
    free_log_lines(loglines, num_lines);
    free(lines);
    if (file_list) free_string_lines(file_list, file_list_count);
    if (file_diff) free_string_lines(file_diff, file_diff_count);
    endwin();
    return exit_code;
}
