#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H

#include "ui.h"
#include "ui_commit.h"
#include <stdio.h>
#include <ncurses.h>
#include <limits.h>

typedef struct { char name[64]; char color[16]; } BranchColorMap;

extern char column_branch[128][64];
extern BranchColorMap branch_color_map[256];
extern int branch_color_map_count;
extern char branch_palette_colors[16][16];
extern int branch_palette_len;
extern int suspend_heavy_render;

char* expand_tabs(const char* input);
char* replace_graph_prefix_preserve_ansi(const char* src, int len, const char* line_style);
char* replace_tree_chars(const char* input, const char* line_style);
int utf8_char_len(unsigned char c);
int is_commit_line(const char *s);
int find_next_commit(char **lines, int num_lines, int start_index, int dir);
char* colorize_graph_prefix_preserve_ansi(const char* src, const char* palette, const char* base_color);
char* colorize_graph_prefix_with_token_colors(const char* src, const char** token_colors, int token_count);
void parse_and_print_ansi_line(WINDOW *win, const char* line, int y, int win_width);
void init_default_branch_palette(void);
void load_branch_palette_from_env(void);
void update_branch_color_map(const char *palette, const char *tree_color);
const char *assign_branch_color(const char *branch_name);
void save_setting(const char* project_root, const char* key, const char* value);
void save_branch_palette_to_config(const char* project_root);

void draw_border(WINDOW *win);
void print_left_panel(WINDOW *win, char **lines, int num_lines, int top_line, int highlight_line, int win_height, int win_width, const char* line_style, const char* current_tree_color, const char* current_branch_palette);
void print_right_panel(WINDOW *win, int highlight_item, int win_height, int win_width);
void print_customize_menu(WINDOW *win, int highlight_item, int win_height, int win_width, const char* current_style, const char* current_border_color, const char* current_tree_color, const char* current_branch_palette);
void print_palette_editor(WINDOW *win, int highlight_item, int win_height, int win_width);
char **get_commit_diff_lines(const char *project_root, const char *hash, int *out_count);
void free_string_lines(char **lines, int count);
void print_preview(WINDOW *win, char **lines, int num_lines, int top_line, int win_height, int win_width, const char *header, int is_list, int selected_index, const char *project_root, const char *commit_hash);
void get_commit_file_char_stats(const char *project_root, const char *hash, const char *path, int *out_added_chars, int *out_removed_chars);
char **get_commit_changed_files(const char *project_root, const char *hash, int *out_count);
char **get_commit_file_diff_lines(const char *project_root, const char *hash, const char *path, int *out_count);

#endif
