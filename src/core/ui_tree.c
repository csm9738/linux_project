#include "ui_internal.h"
#include "parser.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

void print_left_panel(WINDOW *win, char **lines, int num_lines, int top_line, int highlight_line, int win_height, int win_width, const char* line_style, const char* current_tree_color, const char* current_branch_palette) {
    werase(win);
    draw_border(win);
    for (int i = 0; i < win_height - 2; ++i) {
        int current_line = top_line + i;
        if (current_line >= num_lines) break;
        char* expanded_line = expand_tabs(lines[current_line]);
        if (expanded_line == NULL) continue;
        char* processed_line = NULL;
        {
            const char *s = expanded_line;
            int pfx_len = 0;
            while (s[pfx_len] != '\0') {
                if (s[pfx_len] == '\x1b' && s[pfx_len+1] == '[') { pfx_len += 2; while (s[pfx_len] != '\0' && s[pfx_len] != 'm') pfx_len++; if (s[pfx_len] == 'm') pfx_len++; continue; }
                char c = s[pfx_len];
                if (c == ' ' || c == '|' || c == '*' || c == '/' || c == '\\' || c == '+' || c == '-' || c == 'o') { pfx_len++; continue; }
                if (isalnum((unsigned char)c) || c == '(') break;
                break;
            }
            if (pfx_len > 0) {
                char *converted_prefix = replace_graph_prefix_preserve_ansi(s, pfx_len, line_style);
                if (converted_prefix == NULL) { free(expanded_line); continue; }
                int token_count = 0;
                for (size_t ti = 0; ti < strlen(converted_prefix); ) {
                    unsigned char cc = converted_prefix[ti];
                    if (cc == 0x1b && converted_prefix[ti+1] == '[') { size_t jj = ti + 2; while (jj < strlen(converted_prefix) && converted_prefix[jj] != 'm') jj++; if (jj < strlen(converted_prefix) && converted_prefix[jj] == 'm') jj++; ti = jj; continue; }
                    if (converted_prefix[ti] == ' ') { ti++; continue; }
                    token_count++;
                    ti += utf8_char_len(cc);
                }
                const char **token_colors = NULL;
                if (token_count > 0) {
                    token_colors = malloc(sizeof(char*) * token_count);
                    for (int tc = 0; tc < token_count; ++tc) token_colors[tc] = NULL;
                    const char *after = s + pfx_len;
                    const char *open = strchr(after, '(');
                    const char *close = open ? strchr(open, ')') : NULL;
                    int branch_name_count = 0;
                    char branch_tokens[16][64];
                    if (open && close && close > open) {
                        const char *tokstart = open + 1;
                        const char *it = tokstart;
                        int bi = 0;
                        while (it <= close && bi < (int)(sizeof(branch_tokens)/sizeof(branch_tokens[0]))) {
                            if (*it == ',' || it == close) {
                                size_t tlen = it - tokstart;
                                if (tlen >= sizeof(branch_tokens[bi])) tlen = sizeof(branch_tokens[bi]) - 1;
                                memcpy(branch_tokens[bi], tokstart, tlen);
                                branch_tokens[bi][tlen] = '\0';
                                char *tb = branch_tokens[bi]; while (*tb == ' ') tb++; memmove(branch_tokens[bi], tb, strlen(tb)+1);
                                char *te = branch_tokens[bi] + strlen(branch_tokens[bi]) - 1; while (te > branch_tokens[bi] && *te == ' ') *te-- = '\0';
                                branch_name_count++; bi++;
                                if (it == close) break;
                                tokstart = it + 1; it = tokstart; continue;
                            }
                            it++;
                        }
                    }
                    char **token_glyphs = malloc(sizeof(char*) * token_count);
                    int *is_commit_token = calloc(token_count, sizeof(int));
                    int ti_idx = 0;
                    for (size_t ti = 0; ti < strlen(converted_prefix); ) {
                        unsigned char cc = converted_prefix[ti];
                        if (cc == 0x1b && converted_prefix[ti+1] == '[') { size_t jj = ti + 2; while (jj < strlen(converted_prefix) && converted_prefix[jj] != 'm') jj++; if (jj < strlen(converted_prefix) && converted_prefix[jj] == 'm') jj++; ti = jj; continue; }
                        if (converted_prefix[ti] == ' ') { ti++; continue; }
                        int clen = utf8_char_len(converted_prefix[ti]);
                        char *g = malloc(clen + 1);
                        memcpy(g, &converted_prefix[ti], clen);
                        g[clen] = '\0';
                        token_glyphs[ti_idx] = g;
                        if (g[0] == '*' || g[0] == 'o' || strcmp(g, "●") == 0) is_commit_token[ti_idx] = 1;
                        ti += clen; ti_idx++;
                    }
                    if (branch_name_count > 0) {
                        if (open) {
                            int open_offset = open - s;
                            int best_idx = -1;
                            int best_dist = INT_MAX;
                            int token_idx = 0;
                            for (size_t ti = 0; ti < strlen(converted_prefix); ) {
                                unsigned char cc = converted_prefix[ti];
                                if (cc == 0x1b && converted_prefix[ti+1] == '[') { size_t jj = ti + 2; while (jj < strlen(converted_prefix) && converted_prefix[jj] != 'm') jj++; if (jj < strlen(converted_prefix) && converted_prefix[jj] == 'm') jj++; ti = jj; continue; }
                                if (converted_prefix[ti] == ' ') { ti++; continue; }
                                int clen = utf8_char_len(converted_prefix[ti]);
                                int token_byte_pos = ti;
                                if (is_commit_token[token_idx]) {
                                    int dist = abs(token_byte_pos - open_offset);
                                    if (dist < best_dist) { best_dist = dist; best_idx = token_idx; }
                                }
                                ti += clen; token_idx++;
                            }
                            if (best_idx >= 0) {
                                for (int bn = 0; bn < branch_name_count && (best_idx + bn) < token_count; ++bn) {
                                    strncpy(column_branch[best_idx + bn], branch_tokens[bn], sizeof(column_branch[best_idx + bn]) - 1);
                                    column_branch[best_idx + bn][sizeof(column_branch[best_idx + bn]) - 1] = '\0';
                                }
                            }
                        }
                    }
                    for (int tc = 0; tc < token_count; ++tc) {
                        if (column_branch[tc][0] != '\0') {
                            token_colors[tc] = assign_branch_color(column_branch[tc]);
                        } else {
                            if (current_branch_palette && strcmp(current_branch_palette, "rainbow") == 0) token_colors[tc] = branch_palette_colors[tc % branch_palette_len];
                            else if (current_branch_palette && strcmp(current_branch_palette, "alternate") == 0) token_colors[tc] = branch_palette_colors[(tc % 2) + 1];
                            else token_colors[tc] = current_tree_color;
                        }
                    }
                    for (int gi = 0; gi < token_count; ++gi) free(token_glyphs[gi]);
                    free(token_glyphs);
                    free(is_commit_token);
                }
                char *colored_prefix = NULL;
                if (token_colors) colored_prefix = colorize_graph_prefix_with_token_colors(converted_prefix, token_colors, token_count);
                else colored_prefix = colorize_graph_prefix_preserve_ansi(converted_prefix, current_branch_palette, current_tree_color);
                free(converted_prefix);
                if (token_colors) free(token_colors);
                if (colored_prefix == NULL) { free(expanded_line); continue; }
                size_t out_cap = strlen(colored_prefix) + strlen(s + pfx_len) + 1;
                processed_line = malloc(out_cap);
                if (processed_line) {
                    processed_line[0] = '\0';
                    strncat(processed_line, colored_prefix, out_cap - strlen(processed_line) - 1);
                    strncat(processed_line, s + pfx_len, out_cap - strlen(processed_line) - 1);
                }
                free(colored_prefix);
            } else processed_line = replace_tree_chars(expanded_line, line_style);
            free(expanded_line);
            if (processed_line == NULL) continue;
        }
        if (current_line == highlight_line) wattron(win, A_REVERSE);
        char *colorized = NULL;
        {
            const char *p = processed_line;
            const char *open = strchr(p, '(');
            const char *close = open ? strchr(open, ')') : NULL;
            if (open && close && close > open) {
                size_t out_cap = strlen(processed_line) + 256;
                colorized = malloc(out_cap);
                if (colorized) {
                    colorized[0] = '\0';
                    strncat(colorized, p, open - p + 1);
                    char tokenbuf[512];
                    const char *tokstart = open + 1;
                    const char *it = tokstart;
                    while (it <= close) {
                        if (*it == ',' || it == close) {
                            size_t tlen = it - tokstart; if (tlen >= sizeof(tokenbuf)) tlen = sizeof(tokenbuf) - 1;
                            memcpy(tokenbuf, tokstart, tlen); tokenbuf[tlen] = '\0';
                            char *tb = tokenbuf; while (*tb == ' ') tb++; char *te = tb + strlen(tb) - 1; while (te > tb && *te == ' ') *te-- = '\0';
                            unsigned int h = 0; for (char *q = tb; *q; ++q) h = h * 31 + (unsigned char)*q;
                            int fg = (h % 7) + 31;
                            char esc_start[16]; snprintf(esc_start, sizeof(esc_start), "\x1b[%dm", fg);
                            strncat(colorized, esc_start, out_cap - strlen(colorized) - 1);
                            strncat(colorized, tb, out_cap - strlen(colorized) - 1);
                            strncat(colorized, "\x1b[0m", out_cap - strlen(colorized) - 1);
                            if (it == close) break;
                            strncat(colorized, ",", out_cap - strlen(colorized) - 1);
                            tokstart = it + 1; it = tokstart;
                            if (*tokstart == ' ') strncat(colorized, " ", out_cap - strlen(colorized) - 1);
                            continue;
                        }
                        it++;
                    }
                    strncat(colorized, close, out_cap - strlen(colorized) - 1);
                }
            }
        }
        if (colorized) { parse_and_print_ansi_line(win, colorized, i + 1, win_width); free(colorized); }
        else { parse_and_print_ansi_line(win, processed_line, i + 1, win_width); }
        if (current_line == highlight_line) wattroff(win, A_REVERSE);
        free(processed_line);
    }
}
