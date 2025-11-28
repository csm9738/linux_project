#include "ui.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <locale.h>
#include <ctype.h>

#define TAB_WIDTH 8
#define HIGHLIGHT_PAIR 11

static void draw_border(WINDOW *win, int color_pair) {
    wattron(win, COLOR_PAIR(color_pair));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(color_pair));
}

static char* expand_tabs(const char* input) {
    int tab_count = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == '\t') tab_count++;
    }
    char* output = malloc(strlen(input) + tab_count * (TAB_WIDTH - 1) + 1);
    if (output == NULL) return NULL;
    int i = 0, j = 0;
    while (input[i] != '\0') {
        if (input[i] == '\t') {
            for (int k = 0; k < TAB_WIDTH; k++) output[j++] = ' ';
            i++;
        } else {
            output[j++] = input[i++];
        }
    }
    output[j] = '\0';
    return output;
}

static char* replace_tree_chars(const char* input) {
    char* output = malloc(strlen(input) * 4 + 1);
    if (output == NULL) return NULL;
    output[0] = '\0';
    int text_part_started = 0;
    for (int i = 0; input[i] != '\0';) {
        char current_char = input[i];
        if (!text_part_started && (isalnum(current_char) || current_char == '(')) {
            text_part_started = 1;
        }
        if (!text_part_started) {
            if (strncmp(&input[i], "|", 1) == 0) { strcat(output, "│"); i+=1; }
            else if (strncmp(&input[i], "/", 1) == 0) { strcat(output, "╭"); i+=1; }
            else if (strncmp(&input[i], "\\", 1) == 0) { strcat(output, "╰"); i+=1; }
            else if (strncmp(&input[i], "-", 1) == 0) { strcat(output, "─"); i+=1; }
            else if (strncmp(&input[i], "*", 1) == 0) { strcat(output, "●"); i+=1; }
            else { strncat(output, &current_char, 1); i+=1; }
        } else {
            strncat(output, &current_char, 1);
            i+=1;
        }
    }
    return output;
}

static void print_left_panel(WINDOW *win, char **lines, int num_lines, int top_line, int highlight_line, int win_height, int win_width, const char* line_style) {
    werase(win);
    for (int i = 0; i < win_height - 2; ++i) {
        int current_line_index = top_line + i;
        if (current_line_index >= num_lines) break;
        char* expanded_line = expand_tabs(lines[current_line_index]);
        if (expanded_line == NULL) continue;
        char* processed_line = NULL;
        if (line_style != NULL && strcmp(line_style, "unicode") == 0) {
            processed_line = replace_tree_chars(expanded_line);
        } else {
            processed_line = strdup(expanded_line);
        }
        free(expanded_line);
        if (processed_line == NULL) continue;
        if (current_line_index == highlight_line) wattron(win, COLOR_PAIR(HIGHLIGHT_PAIR));
        mvwprintw(win, i + 1, 1, "%.*s", win_width - 2, processed_line);
        if (current_line_index == highlight_line) wattroff(win, COLOR_PAIR(HIGHLIGHT_PAIR));
        free(processed_line);
    }
}

static void print_right_panel(WINDOW *win, const char** menu_items, int num_items, int highlight_item, const char* project_root, const char* current_style) {
    werase(win);
    mvwprintw(win, 1, 1, "Welcome");
    mvwprintw(win, 3, 1, "Press 'q' to quit");
    for (int i = 0; i < num_items; ++i) {
        if (i == highlight_item) wattron(win, COLOR_PAIR(HIGHLIGHT_PAIR));
        mvwprintw(win, 5 + i, 1, "%s", menu_items[i]);
        if (i == highlight_item) wattroff(win, COLOR_PAIR(HIGHLIGHT_PAIR));
    }
    
    char config_path[1024];
    snprintf(config_path, sizeof(config_path), "%s/config/gitscope.conf", project_root);
    const char* env_style = getenv("LINE_STYLE");

    mvwprintw(win, 10, 1, "--- DEBUG INFO ---");
    mvwprintw(win, 11, 1, "Project Root: %.*s", 40, project_root);
    mvwprintw(win, 12, 1, "Config Path: %.*s", 40, config_path);
    mvwprintw(win, 13, 1, "getenv(LINE_STYLE): %s", env_style ? env_style : "null");
    mvwprintw(win, 14, 1, "Internal Style: %s", current_style);
}

static void print_customize_menu(WINDOW *win, int highlight_item, const char* current_style) {
    werase(win);
    mvwprintw(win, 1, 1, "--- Customize Tree ---");
    const char *menu_items[] = {"Line Style: ASCII", "Line Style: Unicode", "[Back]"};
    for (int i = 0; i < 3; ++i) {
        char menu_display[256];
        int is_current = 0;
        if (i == 0 && strcmp(current_style, "ascii") == 0) is_current = 1;
        if (i == 1 && strcmp(current_style, "unicode") == 0) is_current = 1;
        snprintf(menu_display, sizeof(menu_display), "%s %s", menu_items[i], is_current ? "(*)" : "");
        if (i == highlight_item) wattron(win, A_REVERSE);
        mvwprintw(win, 3 + i, 1, "%s", menu_items[i]);
        if (i == highlight_item) wattroff(win, A_REVERSE);
    }
}

static void save_setting(const char* project_root, const char* key, const char* value) {
    char config_path[1024];
    snprintf(config_path, sizeof(config_path), "%s/config", project_root);
    mkdir(config_path, 0755);
    snprintf(config_path, sizeof(config_path), "%s/config/gitscope.conf", project_root);
    FILE* fp = fopen(config_path, "w");
    if (fp != NULL) {
        fprintf(fp, "%s=%s\n", key, value);
        fclose(fp);
    }
}

int start_ui(const char* git_log_filepath, const char* project_root) {
    setlocale(LC_ALL, "");
    initscr();
    if (has_colors() == FALSE) { endwin(); return 1; }
    start_color();
    use_default_colors();
    init_pair(1, COLOR_WHITE, -1);
    init_pair(2, COLOR_CYAN, -1);
    init_pair(HIGHLIGHT_PAIR, COLOR_BLACK, COLOR_WHITE);
    clear(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);

    int height, width;
    getmaxyx(stdscr, height, width);
    if (width < 10) { endwin(); return 1; }
    refresh();

    WINDOW *left_win = newwin(height, width / 2, 0, 0);
    WINDOW *right_win = newwin(height, width / 2, 0, width / 2);
    
    FILE *fp = fopen(git_log_filepath, "r");
    if (fp == NULL) { endwin(); return 1; }
    int capacity = 20, num_lines = 0;
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

    const char *main_menu_items[] = {"Customize Tree", "Run Tests", "Commit"};
    int num_main_menu_items = sizeof(main_menu_items) / sizeof(char*);
    const char *customize_menu_items[] = {"Line Style: ASCII", "Line Style: Unicode", "[Back]"};
    int num_customize_menu_items = sizeof(customize_menu_items) / sizeof(char*);

    char current_line_style[20] = "ascii";
    const char* env_line_style = getenv("LINE_STYLE");
    if (env_line_style != NULL) {
        strncpy(current_line_style, env_line_style, sizeof(current_line_style) - 1);
    }

    int left_highlight = 0, top_line = 0, right_highlight = 0, active_window = 0, ch, exit_code = 0;
    ScreenState current_screen = MAIN_SCREEN;
    int should_exit = 0;
    
    while (!should_exit) {
        print_left_panel(left_win, lines, num_lines, top_line, (active_window == 0) ? left_highlight : -1, height, width / 2, current_line_style);
        if (current_screen == MAIN_SCREEN) {
            print_right_panel(right_win, main_menu_items, num_main_menu_items, (active_window == 1) ? right_highlight : -1, project_root, current_line_style);
        } else if (current_screen == CUSTOMIZE_SCREEN) {
            print_customize_menu(right_win, (active_window == 1) ? right_highlight : -1, current_line_style);
        }
        draw_border(left_win, (active_window == 0) ? 2 : 1);
        draw_border(right_win, (active_window == 1) ? 2 : 1);
        
        wnoutrefresh(left_win);
        wnoutrefresh(right_win);
        doupdate();

        ch = getch();
        if (ch == 'q') {
            should_exit = 1;
            continue;
        }

        if (ch == KEY_LEFT || ch == 'h') active_window = 0;
        else if (ch == KEY_RIGHT || ch == 'l') active_window = 1;
        else if (active_window == 0) {
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
        } else {
            if (current_screen == MAIN_SCREEN) {
                switch(ch) {
                    case KEY_UP: case 'k':
                        if (right_highlight > 0) right_highlight--;
                        break;
                    case KEY_DOWN: case 'j':
                        if (right_highlight < num_main_menu_items - 1) right_highlight++;
                        break;
                    case '\n':
                        if (right_highlight == 0) {
                            current_screen = CUSTOMIZE_SCREEN;
                            right_highlight = 0;
                        } else if (right_highlight == 1) {
                            exit_code = 2; should_exit = 1;
                        } else if (right_highlight == 2) {
                            exit_code = 3; should_exit = 1;
                        }
                        break;
                }
            } else if (current_screen == CUSTOMIZE_SCREEN) {
                switch(ch) {
                    case KEY_UP: case 'k':
                        if (right_highlight > 0) right_highlight--;
                        break;
                    case KEY_DOWN: case 'j':
                        if (right_highlight < num_customize_menu_items - 1) right_highlight++;
                        break;
                    case '\n':
                        if (right_highlight == 0) {
                            save_setting(project_root, "LINE_STYLE", "ascii");
                            strcpy(current_line_style, "ascii");
                        } else if (right_highlight == 1) {
                            save_setting(project_root, "LINE_STYLE", "unicode");
                            strcpy(current_line_style, "unicode");
                        } else if (right_highlight == 2) {
                            current_screen = MAIN_SCREEN;
                            right_highlight = 0;
                        }
                        break;
                }
            }
        }
    }

    for (int i = 0; i < num_lines; ++i) free(lines[i]);
    free(lines);
    endwin();
    return exit_code;
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    return start_ui(argv[1], argv[2]);
}