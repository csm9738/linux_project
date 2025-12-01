#include "ui_internal.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

void print_right_panel(WINDOW *win, int highlight_item, int win_height, int win_width) {
    werase(win);
    draw_border(win);
    mvwprintw(win, 1, 2, "Welcome to gitscope!");
    const char *menu_items[] = {"Customize Style", "Commit"};
    for (int i = 0; i < 2; ++i) {
        if (i == highlight_item) wattron(win, A_REVERSE);
        mvwprintw(win, 2 + i, 3, "%s", menu_items[i]);
        if (i == highlight_item) wattroff(win, A_REVERSE);
    }
}

void print_customize_menu(WINDOW *win, int highlight_item, int win_height, int win_width, const char* current_style, const char* current_border_color, const char* current_tree_color, const char* current_branch_palette) {
    werase(win);
    draw_border(win);
    mvwprintw(win, 1, 2, "--- Customize ---");
    char style_line[128]; snprintf(style_line, sizeof(style_line), "Tree Style: %s", current_style ? current_style : "ascii");
    char border_line[128]; snprintf(border_line, sizeof(border_line), "Highlight Color: %s", current_border_color ? current_border_color : "cyan");
    char tree_line[128]; snprintf(tree_line, sizeof(tree_line), "Tree Color: %s", current_tree_color ? current_tree_color : "green");
    char palette_line[128]; snprintf(palette_line, sizeof(palette_line), "Branch Palette: %s", current_branch_palette ? current_branch_palette : "single");
    char edit_palette_line[128]; snprintf(edit_palette_line, sizeof(edit_palette_line), "Edit Palette Colors");
    const char *menu_items[] = { style_line, border_line, tree_line, palette_line, edit_palette_line, "[Back] (saves)" };
    int num_items = sizeof(menu_items) / sizeof(char*);
    for (int i = 0; i < num_items; ++i) {
        if (i == highlight_item) wattron(win, A_REVERSE);
        mvwprintw(win, 3 + i, 3, "%s", menu_items[i]);
        if (i == highlight_item) wattroff(win, A_REVERSE);
    }
}

void print_palette_editor(WINDOW *win, int highlight_item, int win_height, int win_width) {
    werase(win);
    draw_border(win);
    mvwprintw(win, 1, 2, "--- Edit Palette Colors ---");
    int base_y = 3;
    for (int i = 0; i < branch_palette_len; ++i) {
        if (i == highlight_item) wattron(win, A_REVERSE);
        mvwprintw(win, base_y + i, 3, "Slot %d: %s", i+1, branch_palette_colors[i]);
        if (i == highlight_item) wattroff(win, A_REVERSE);
    }
    int back_idx = branch_palette_len;
    if (highlight_item == back_idx) wattron(win, A_REVERSE);
    mvwprintw(win, base_y + back_idx, 3, "[Back] (saves)");
    if (highlight_item == back_idx) wattroff(win, A_REVERSE);
}

void print_preview(WINDOW *win, char **lines, int num_lines, int top_line, int win_height, int win_width, const char *header, int is_list, int selected_index, const char *project_root, const char *commit_hash) {
    werase(win);
    draw_border(win);
    if (header) {
        wattron(win, A_BOLD);
        mvwprintw(win, 1, 2, "%s", header);
        wattroff(win, A_BOLD);
    }
    if (suspend_heavy_render) {
        /* Lightweight preview rendering during resize: no git calls, no ANSI parsing */
        int content_start = 2;
        int content_h = win_height - content_start - 0;
        int maxw = win_width - 4;
        for (int i = 0; i < content_h; ++i) {
            int idx = top_line + i;
            if (idx >= num_lines) break;
            int y = content_start + i;
            if (is_list) {
                if (idx == selected_index) wattron(win, A_REVERSE);
                /* print filename only, truncated */
                char buf[1024]; buf[0] = '\0';
                strncpy(buf, lines[idx] ? lines[idx] : "", maxw); buf[maxw] = '\0';
                mvwprintw(win, y, 2, "%s", buf);
                if (idx == selected_index) wattroff(win, A_REVERSE);
            } else {
                /* show raw line without parsing */
                char buf[1024]; buf[0] = '\0';
                strncpy(buf, lines[idx] ? lines[idx] : "", maxw); buf[maxw] = '\0';
                mvwprintw(win, y, 2, "%s", buf);
            }
        }
        return;
    }
    int content_start = 2;
    int content_h = win_height - content_start - 0;
    for (int i = 0; i < content_h; ++i) {
        int idx = top_line + i;
        if (idx >= num_lines) break;
        int y = content_start + i;
        if (is_list) {
            int added_chars = 0, removed_chars = 0;
            if (project_root && commit_hash && lines[idx]) {
                get_commit_file_char_stats(project_root, commit_hash, lines[idx], &added_chars, &removed_chars);
            }
            if (idx == selected_index) wattron(win, A_REVERSE);
            mvwprintw(win, y, 2, "%s  (+%d, -%d)", lines[idx], added_chars, removed_chars);
            if (idx == selected_index) wattroff(win, A_REVERSE);
        } else {
            parse_and_print_ansi_line(win, lines[idx], y, win_width);
        }
    }
}
