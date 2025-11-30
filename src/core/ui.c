#include "ui.h"
#include "parser.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <locale.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>

#define HIGHLIGHT_PAIR 11
#define BORDER_PAIR 12
#define TAB_WIDTH 8

static char column_branch[128][64];
struct BranchColorMap { char name[64]; char color[16]; };
static struct BranchColorMap branch_color_map[256];
static int branch_color_map_count = 0;

static char branch_palette_colors[16][16];
static int branch_palette_len = 0;

static void init_default_branch_palette(void) {
    const char *defaults[] = {"red","green","yellow","blue","magenta","cyan"};
    int n = sizeof(defaults)/sizeof(defaults[0]);
    for (int i = 0; i < n; ++i) {
        strncpy(branch_palette_colors[i], defaults[i], sizeof(branch_palette_colors[i]) - 1);
        branch_palette_colors[i][sizeof(branch_palette_colors[i]) - 1] = '\0';
    }
    branch_palette_len = n;
}

static void load_branch_palette_from_env(void) {
    const char *env = getenv("BRANCH_PALETTE_COLORS");
    if (!env) { init_default_branch_palette(); return; }
    char tmp[1024]; strncpy(tmp, env, sizeof(tmp)-1); tmp[sizeof(tmp)-1] = '\0';
    char *saveptr = NULL;
    char *tok = strtok_r(tmp, ",", &saveptr);
    int idx = 0;
    while (tok && idx < (int)(sizeof(branch_palette_colors)/sizeof(branch_palette_colors[0]))) {
        char *ttrim = tok;
        while (*ttrim == ' ') ttrim++;
        strncpy(branch_palette_colors[idx], ttrim, sizeof(branch_palette_colors[idx]) - 1);
        branch_palette_colors[idx][sizeof(branch_palette_colors[idx]) - 1] = '\0';
        idx++; tok = strtok_r(NULL, ",", &saveptr);
    }
    if (idx == 0) init_default_branch_palette(); else branch_palette_len = idx;
}

static void update_branch_color_map(const char *palette, const char *tree_color) {
    for (int i = 0; i < branch_color_map_count; ++i) {
        if (palette && strcmp(palette, "single") == 0) {
            strncpy(branch_color_map[i].color, tree_color ? tree_color : "cyan", sizeof(branch_color_map[i].color)-1);
            branch_color_map[i].color[sizeof(branch_color_map[i].color)-1] = '\0';
        } else if (palette && strcmp(palette, "rainbow") == 0) {
            const char *col = branch_palette_colors[i % branch_palette_len];
            strncpy(branch_color_map[i].color, col, sizeof(branch_color_map[i].color)-1);
            branch_color_map[i].color[sizeof(branch_color_map[i].color)-1] = '\0';
        } else if (palette && strcmp(palette, "alternate") == 0) {
            const char *col = branch_palette_colors[(i % 2) + 1];
            strncpy(branch_color_map[i].color, col, sizeof(branch_color_map[i].color)-1);
            branch_color_map[i].color[sizeof(branch_color_map[i].color)-1] = '\0';
        }
    }
}

static const char *assign_branch_color(const char *branch_name) {
    if (!branch_name) return "cyan";
    for (int i = 0; i < branch_color_map_count; ++i) {
        if (strcmp(branch_color_map[i].name, branch_name) == 0) return branch_color_map[i].color;
    }
    unsigned int h = 0;
    for (const char *p = branch_name; *p; ++p) h = h * 31 + (unsigned char)*p;
    const char *col = branch_palette_colors[h % branch_palette_len];
    if (branch_color_map_count < (int)(sizeof(branch_color_map)/sizeof(branch_color_map[0]))) {
        strncpy(branch_color_map[branch_color_map_count].name, branch_name, sizeof(branch_color_map[branch_color_map_count].name) - 1);
        strncpy(branch_color_map[branch_color_map_count].color, col, sizeof(branch_color_map[branch_color_map_count].color) - 1);
        branch_color_map[branch_color_map_count].name[sizeof(branch_color_map[branch_color_map_count].name)-1] = '\0';
        branch_color_map[branch_color_map_count].color[sizeof(branch_color_map[branch_color_map_count].color)-1] = '\0';
        branch_color_map_count++;
    }
    return col;
}


static void draw_border(WINDOW *win) {
    box(win, 0, 0);
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

static char* replace_tree_chars(const char* input, const char* line_style) {
    char* output = malloc(strlen(input) * 4 + 1);
    if (output == NULL) return NULL;
    output[0] = '\0';
    int text_part_started = 0;

    const char* V = "|";
    const char* SL = "/";
    const char* BS = "\\";
    const char* H = "-";
    const char* S = "*";

    if (line_style != NULL) {
        if (strcmp(line_style, "unicode") == 0) {
            V = "│"; SL = "╭"; BS = "╰"; H = "─"; S = "●";
        } else if (strcmp(line_style, "unicode-double") == 0) {
            V = "║"; SL = "╔"; BS = "╚"; H = "═"; S = "●";
        } else if (strcmp(line_style, "unicode-rounded") == 0) {
            V = "ㅣ"; SL = " r"; BS = "ㄴ"; H = "ㅡ"; S = "*";
        }
    }

    for (size_t i = 0; i < strlen(input); i++) {
        char current_char = input[i];
        if (!text_part_started && (isalnum(current_char) || current_char == '(')) {
            text_part_started = 1;
        }

        if (!text_part_started) {
            if (current_char == '|') { strcat(output, V); }
            else if (current_char == '/') { strcat(output, SL); }
            else if (current_char == '\\') { strcat(output, BS); }
            else if (current_char == '-') { strcat(output, H); }
            else if (current_char == '*') { strcat(output, S); }
            else { strncat(output, &current_char, 1); }
        } else {
            strncat(output, &current_char, 1);
        }
    }
    return output;
}

static char* replace_graph_prefix_preserve_ansi(const char* src, int len, const char* line_style) {
    const char* V = "|";
    const char* SL = "/";
    const char* BS = "\\";
    const char* H = "-";
    const char* S = "*";
    if (line_style != NULL) {
        if (strcmp(line_style, "unicode") == 0) {
            V = "│"; SL = "╭"; BS = "╰"; H = "─"; S = "●";
        } else if (strcmp(line_style, "unicode-double") == 0) {
            V = "║"; SL = "╔"; BS = "╚"; H = "═"; S = "●";
        } else if (strcmp(line_style, "unicode-rounded") == 0) {
            V = "ㅣ"; SL = " r"; BS = "ㄴ"; H = "ㅡ"; S = "*";
        }
    }

    size_t out_cap = len * 4 + 16;
    char* out = malloc(out_cap);
    if (!out) return NULL;
    out[0] = '\0';

    for (int i = 0; i < len; ) {
        unsigned char c = src[i];
        if (c == 0x1b && src[i+1] == '[') {
            int j = i + 2;
            while (j < len && src[j] != 'm') j++;
            if (j < len && src[j] == 'm') j++;
            strncat(out, &src[i], j - i);
            i = j;
            continue;
        }

        if (c == '|' ) { strncat(out, V, out_cap - strlen(out) - 1); i++; continue; }
        if (c == '/') { strncat(out, SL, out_cap - strlen(out) - 1); i++; continue; }
        if (c == '\\') { strncat(out, BS, out_cap - strlen(out) - 1); i++; continue; }
        if (c == '-') { strncat(out, H, out_cap - strlen(out) - 1); i++; continue; }
        if (c == '*') { strncat(out, S, out_cap - strlen(out) - 1); i++; continue; }
        strncat(out, (const char*)&src[i], 1);
        i++;
    }
    return out;
}

static int utf8_char_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static char* colorize_graph_prefix_preserve_ansi(const char* src, const char* palette, const char* base_color) {
    if (!src) return NULL;
    size_t n = strlen(src);
    size_t out_cap = n * 6 + 64;
    char* out = malloc(out_cap);
    if (!out) return NULL;
    out[0] = '\0';

    const char *palette_colors[] = {"red","green","yellow","blue","magenta","cyan"};
    int palette_len = sizeof(palette_colors) / sizeof(palette_colors[0]);
    int pos = 0;

    for (size_t i = 0; i < n; ) {
        unsigned char c = src[i];
        if (c == 0x1b && i + 1 < n && src[i+1] == '[') {
            size_t j = i + 2;
            while (j < n && src[j] != 'm') j++;
            if (j < n && src[j] == 'm') j++;
            strncat(out, &src[i], j - i);
            i = j;
            continue;
        }

        if (src[i] == ' ') {
            strncat(out, " ", out_cap - strlen(out) - 1);
            i++;
            continue;
        }

        const char *color_name = base_color ? base_color : "cyan";
        if (palette && strcmp(palette, "rainbow") == 0) {
            color_name = palette_colors[pos % palette_len];
        } else if (palette && strcmp(palette, "alternate") == 0) {
            color_name = palette_colors[(pos % 2) + 1];
        } else if (palette && strcmp(palette, "single") == 0) {
            color_name = base_color ? base_color : "cyan";
        }

        int fg = 37;
        if (strcmp(color_name, "black") == 0) fg = 30;
        else if (strcmp(color_name, "red") == 0) fg = 31;
        else if (strcmp(color_name, "green") == 0) fg = 32;
        else if (strcmp(color_name, "yellow") == 0) fg = 33;
        else if (strcmp(color_name, "blue") == 0) fg = 34;
        else if (strcmp(color_name, "magenta") == 0) fg = 35;
        else if (strcmp(color_name, "cyan") == 0) fg = 36;
        else fg = 37;

        char esc_start[16];
        snprintf(esc_start, sizeof(esc_start), "\x1b[%dm", fg);
        strncat(out, esc_start, out_cap - strlen(out) - 1);

        int clen = utf8_char_len(c);
        if (i + clen > n) clen = 1;
        strncat(out, &src[i], clen);
        strncat(out, "\x1b[0m", out_cap - strlen(out) - 1);
        i += clen;
        pos++;
    }

    return out;
}

static char* colorize_graph_prefix_with_token_colors(const char* src, const char** token_colors, int token_count) {
    if (!src) return NULL;
    size_t n = strlen(src);
    size_t out_cap = n * 6 + 64;
    char* out = malloc(out_cap);
    if (!out) return NULL;
    out[0] = '\0';
    int pos = 0;

    for (size_t i = 0; i < n; ) {
        unsigned char c = src[i];
        if (c == 0x1b && i + 1 < n && src[i+1] == '[') {
            size_t j = i + 2;
            while (j < n && src[j] != 'm') j++;
            if (j < n && src[j] == 'm') j++;
            strncat(out, &src[i], j - i);
            i = j;
            continue;
        }
        if (src[i] == ' ') {
            strncat(out, " ", out_cap - strlen(out) - 1);
            i++; continue;
        }
        const char *color_name = (pos < token_count && token_colors && token_colors[pos]) ? token_colors[pos] : "cyan";
        int fg = 37;
        if (strcmp(color_name, "black") == 0) fg = 30;
        else if (strcmp(color_name, "red") == 0) fg = 31;
        else if (strcmp(color_name, "green") == 0) fg = 32;
        else if (strcmp(color_name, "yellow") == 0) fg = 33;
        else if (strcmp(color_name, "blue") == 0) fg = 34;
        else if (strcmp(color_name, "magenta") == 0) fg = 35;
        else if (strcmp(color_name, "cyan") == 0) fg = 36;
        else fg = 37;
        char esc_start[16]; snprintf(esc_start, sizeof(esc_start), "\x1b[%dm", fg);
        strncat(out, esc_start, out_cap - strlen(out) - 1);

        int clen = utf8_char_len(c);
        if (i + clen > n) clen = 1;
        strncat(out, &src[i], clen);
        strncat(out, "\x1b[0m", out_cap - strlen(out) - 1);
        i += clen; pos++;
    }

    return out;
}

void parse_and_print_ansi_line(WINDOW *win, const char* line, int y, int win_width) {
    wmove(win, y, 1);
    int current_attrs = A_NORMAL;
    int max_content_x = win_width - 2;

    for (int i = 0; line[i] != '\0'; ) {
        if (getcurx(win) > max_content_x) break;

        if (line[i] == '\x1b' && line[i+1] == '[') {
            i += 2;
            int attr = A_NORMAL;
            int color_pair = 0;

            char code_buffer[128];
            int j = 0;
            while (line[i] != '\0' && line[i] != 'm' && j < (int)sizeof(code_buffer) - 1) {
                code_buffer[j++] = line[i++];
            }
            code_buffer[j] = '\0';

            if (line[i] != 'm') {
                while (line[i] != '\0' && line[i] != 'm') i++;
            }

            if (j == 0) { /* ESC[m */
                wattroff(win, current_attrs);
                current_attrs = A_NORMAL;
                if (line[i] == 'm') i++;
                continue;
            }

            char *saveptr = NULL;
            char *code = strtok_r(code_buffer, ";", &saveptr);
            while (code != NULL) {
                int value = atoi(code);
                if (value == 0) {
                    attr = A_NORMAL;
                    color_pair = 0;
                } else if (value == 1) {
                    attr |= A_BOLD;
                } else if (value >= 30 && value <= 37) {
                    color_pair = value - 30 + 1;
                } else if (value >= 90 && value <= 97) {
                    color_pair = value - 90 + 1;
                } else if (value == 39) {
                    color_pair = 0;
                } else if (value == 38 || value == 48) {
                    char *next = strtok_r(NULL, ";", &saveptr);
                    if (next != NULL && atoi(next) == 5) {
                        char *param = strtok_r(NULL, ";", &saveptr);
                        if (param != NULL) {
                            int n = atoi(param);
                            color_pair = (n % 8) + 1;
                        }
                    }
                }
                code = strtok_r(NULL, ";", &saveptr);
            }

            wattroff(win, current_attrs);
            current_attrs = attr | COLOR_PAIR(color_pair);
            wattron(win, current_attrs);

            if (line[i] == 'm') i++;
            continue;
        }

        int start = i;
        while (line[i] != '\0' && !(line[i] == '\x1b' && line[i+1] == '[')) i++;
        int len = i - start;
        int curx = getcurx(win);
        int available_cols = max_content_x - curx + 1;
        if (available_cols <= 0) break;
        if (len > available_cols) len = available_cols;
        if (len > 0) {
            waddnstr(win, &line[start], len);
        }
    }
    wattroff(win, current_attrs);
}

static void print_left_panel(WINDOW *win, char **lines, int num_lines, int top_line, int highlight_line, int win_height, int win_width, const char* line_style, const char* current_tree_color, const char* current_branch_palette) {
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
                if (s[pfx_len] == '\x1b' && s[pfx_len+1] == '[') {
                    pfx_len += 2;
                    while (s[pfx_len] != '\0' && s[pfx_len] != 'm') pfx_len++;
                    if (s[pfx_len] == 'm') pfx_len++;
                    continue;
                }
                char c = s[pfx_len];
                if (c == ' ' || c == '|' || c == '*' || c == '/' || c == '\\' || c == '+' || c == '-' || c == 'o') {
                    pfx_len++; continue;
                }
                if (isalnum((unsigned char)c) || c == '(') break;
                break;
            }

            if (pfx_len > 0) {
                char *converted_prefix = replace_graph_prefix_preserve_ansi(s, pfx_len, line_style);
                if (converted_prefix == NULL) {
                    free(expanded_line);
                    continue;
                }
                int token_count = 0;
                for (size_t ti = 0; ti < strlen(converted_prefix); ) {
                    unsigned char cc = converted_prefix[ti];
                    if (cc == 0x1b && converted_prefix[ti+1] == '[') {
                        size_t jj = ti + 2; while (jj < strlen(converted_prefix) && converted_prefix[jj] != 'm') jj++; if (jj < strlen(converted_prefix) && converted_prefix[jj] == 'm') jj++; ti = jj; continue;
                    }
                    if (converted_prefix[ti] == ' ') { ti++; continue; }
                    token_count++;
                    ti += utf8_char_len(cc);
                }

                const char **token_colors = NULL;
                if (token_count > 0) {
                    token_colors = malloc(sizeof(char*) * token_count);
                    for (int tc = 0; tc < token_count; ++tc) token_colors[tc] = NULL;

                    /* attempt to parse branch names from the full line (text after prefix) */
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
                                /* trim */
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
                        if (cc == 0x1b && converted_prefix[ti+1] == '[') {
                            size_t jj = ti + 2; while (jj < strlen(converted_prefix) && converted_prefix[jj] != 'm') jj++; if (jj < strlen(converted_prefix) && converted_prefix[jj] == 'm') jj++; ti = jj; continue;
                        }
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
                                if (cc == 0x1b && converted_prefix[ti+1] == '[') {
                                    size_t jj = ti + 2; while (jj < strlen(converted_prefix) && converted_prefix[jj] != 'm') jj++; if (jj < strlen(converted_prefix) && converted_prefix[jj] == 'm') jj++; ti = jj; continue;
                                }
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
                                if (current_branch_palette && strcmp(current_branch_palette, "rainbow") == 0) {
                                    token_colors[tc] = branch_palette_colors[tc % branch_palette_len];
                                } else if (current_branch_palette && strcmp(current_branch_palette, "alternate") == 0) {
                                    token_colors[tc] = branch_palette_colors[(tc % 2) + 1];
                                } else {
                                    token_colors[tc] = current_tree_color;
                                }
                            }
                    }

                    for (int gi = 0; gi < token_count; ++gi) free(token_glyphs[gi]);
                    free(token_glyphs);
                    free(is_commit_token);
                }

                char *colored_prefix = NULL;
                if (token_colors) {
                    colored_prefix = colorize_graph_prefix_with_token_colors(converted_prefix, token_colors, token_count);
                } else {
                    colored_prefix = colorize_graph_prefix_preserve_ansi(converted_prefix, current_branch_palette, current_tree_color);
                }
                free(converted_prefix);
                if (token_colors) free(token_colors);

                if (colored_prefix == NULL) {
                    free(expanded_line);
                    continue;
                }

                size_t out_cap = strlen(colored_prefix) + strlen(s + pfx_len) + 1;
                processed_line = malloc(out_cap);
                if (processed_line) {
                    processed_line[0] = '\0';
                    strncat(processed_line, colored_prefix, out_cap - strlen(processed_line) - 1);
                    strncat(processed_line, s + pfx_len, out_cap - strlen(processed_line) - 1);
                }
                free(colored_prefix);
            } else {
                processed_line = replace_tree_chars(expanded_line, line_style);
            }

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
                /* build new string with colored branch tokens */
                size_t out_cap = strlen(processed_line) + 256;
                colorized = malloc(out_cap);
                if (colorized) {
                    colorized[0] = '\0';
                    strncat(colorized, p, open - p + 1); /* include '(' */
                    /* copy and colorize comma-separated tokens between open+1 and close-1 */
                    char tokenbuf[512];
                    (void)(open); /* suppress unused calculation */
                    const char *tokstart = open + 1;
                    const char *it = tokstart;
                    while (it <= close) {
                        if (*it == ',' || it == close) {
                            size_t tlen = it - tokstart;
                            if (tlen >= sizeof(tokenbuf)) tlen = sizeof(tokenbuf) - 1;
                            memcpy(tokenbuf, tokstart, tlen);
                            tokenbuf[tlen] = '\0';
                            /* trim leading/trailing spaces */
                            char *tb = tokenbuf;
                            while (*tb == ' ') tb++;
                            char *te = tb + strlen(tb) - 1;
                            while (te > tb && *te == ' ') *te-- = '\0';

                            /* compute simple hash to pick a color (avoid black = pair 1) */
                            unsigned int h = 0;
                            for (char *q = tb; *q; ++q) h = h * 31 + (unsigned char)*q;
                            int fg = (h % 7) + 31; /* 31..37 */

                            char esc_start[16];
                            snprintf(esc_start, sizeof(esc_start), "\x1b[%dm", fg);
                            strncat(colorized, esc_start, out_cap - strlen(colorized) - 1);
                            strncat(colorized, tb, out_cap - strlen(colorized) - 1);
                            strncat(colorized, "\x1b[0m", out_cap - strlen(colorized) - 1);

                            if (it == close) break;
                            /* append comma and space */
                            strncat(colorized, ",", out_cap - strlen(colorized) - 1);
                            /* advance past comma */
                            tokstart = it + 1;
                            it = tokstart;
                            /* append space if present */
                            if (*tokstart == ' ') strncat(colorized, " ", out_cap - strlen(colorized) - 1);
                            continue;
                        }
                        it++;
                    }
                    /* append the rest including ')' */
                    strncat(colorized, close, out_cap - strlen(colorized) - 1);
                }
            }
        }
        if (colorized) {
            parse_and_print_ansi_line(win, colorized, i + 1, win_width);
            free(colorized);
        } else {
            parse_and_print_ansi_line(win, processed_line, i + 1, win_width);
        }
        if (current_line == highlight_line) wattroff(win, A_REVERSE);
        
        free(processed_line);
    }
}

static void print_right_panel(WINDOW *win, int highlight_item, int win_height, int win_width) {
    werase(win);
    draw_border(win);
    mvwprintw(win, 1, 2, "Welcome to gitscope!");
    
    const char *menu_items[] = {"Customize Style", "Run Tests", "Commit"};
    for (int i = 0; i < 3; ++i) {
        if (i == highlight_item) wattron(win, A_REVERSE);
        mvwprintw(win, 3 + i, 3, "%s", menu_items[i]);
        if (i == highlight_item) wattroff(win, A_REVERSE);
    }
}

static void print_customize_menu(WINDOW *win, int highlight_item, int win_height, int win_width, const char* current_style, const char* current_border_color, const char* current_tree_color, const char* current_branch_palette) {
    werase(win);
    draw_border(win);
    mvwprintw(win, 1, 2, "--- Customize ---");

    char style_line[128];
    snprintf(style_line, sizeof(style_line), "Tree Style: %s", current_style ? current_style : "ascii");
    char border_line[128];
    snprintf(border_line, sizeof(border_line), "Highlight Color: %s", current_border_color ? current_border_color : "cyan");
    char tree_line[128];
    snprintf(tree_line, sizeof(tree_line), "Tree Color: %s", current_tree_color ? current_tree_color : "green");
    char palette_line[128];
    snprintf(palette_line, sizeof(palette_line), "Branch Palette: %s", current_branch_palette ? current_branch_palette : "single");
    char edit_palette_line[128];
    snprintf(edit_palette_line, sizeof(edit_palette_line), "Edit Palette Colors");

    /* indicate that returning also saves settings */
    const char *menu_items[] = { style_line, border_line, tree_line, palette_line, edit_palette_line, "[Back] (saves)" };
    int num_items = sizeof(menu_items) / sizeof(char*);

    for (int i = 0; i < num_items; ++i) {
        if (i == highlight_item) wattron(win, A_REVERSE);
        mvwprintw(win, 3 + i, 3, "%s", menu_items[i]);
        if (i == highlight_item) wattroff(win, A_REVERSE);
    }
}

static void save_setting(const char* project_root, const char* key, const char* value) {
    char config_dir_path[1024];
    snprintf(config_dir_path, sizeof(config_dir_path), "%s/config", project_root);
    mkdir(config_dir_path, 0755);
    
    char config_file_path[1024];
    snprintf(config_file_path, sizeof(config_file_path), "%s/config/gitscope.conf", project_root);
    
    /* Read existing config (if any), remove any existing line for this key,
       then write back all other lines and append the new export line.
       This prevents overwriting unrelated settings when saving a single key. */
    char tmp_path[1200];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", config_file_path);

    FILE *in = fopen(config_file_path, "r");
    FILE *out = fopen(tmp_path, "w");
    if (out) {
        if (in) {
            char line[2048];
            size_t keylen = strlen(key);
            while (fgets(line, sizeof(line), in)) {
                if (strncmp(line, "export ", 7) == 0) {
                    /* parse name after 'export ' up to '=' */
                    char *eq = strchr(line + 7, '=');
                    if (eq) {
                        size_t namelen = (size_t)(eq - (line + 7));
                        if (namelen == keylen && strncmp(line + 7, key, keylen) == 0) {
                            /* skip this line (we'll write updated value below) */
                            continue;
                        }
                    }
                }
                fputs(line, out);
            }
            fclose(in);
        }

        fprintf(out, "export %s=%s\n", key, value);
        fclose(out);

        /* atomically replace config file */
        rename(tmp_path, config_file_path);
    } else {
        /* fallback: write directly if temp couldn't be created */
        FILE* fp = fopen(config_file_path, "w");
        if (fp != NULL) {
            fprintf(fp, "export %s=%s\n", key, value);
            fclose(fp);
        }
    }
}

static void save_branch_palette_to_config(const char* project_root) {
    char buf[1024]; buf[0] = '\0';
    for (int i = 0; i < branch_palette_len; ++i) {
        if (i) strncat(buf, ",", sizeof(buf) - strlen(buf) - 1);
        strncat(buf, branch_palette_colors[i], sizeof(buf) - strlen(buf) - 1);
    }
    save_setting(project_root, "BRANCH_PALETTE_COLORS", buf);
}


static void print_palette_editor(WINDOW *win, int highlight_item, int win_height, int win_width) {
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

int start_ui(const char* git_log_filepath, const char* project_root) {
    setlocale(LC_ALL, "");
    initscr();

    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color\n");
        return 1;
    }

    start_color();
    use_default_colors();
    init_pair(HIGHLIGHT_PAIR, COLOR_BLACK, COLOR_WHITE);

    init_pair(1, COLOR_WHITE, -1);
    init_pair(2, COLOR_RED, -1);
    init_pair(3, COLOR_GREEN, -1);
    init_pair(4, COLOR_YELLOW, -1);
    init_pair(5, COLOR_BLUE, -1);
    init_pair(6, COLOR_MAGENTA, -1);
    init_pair(7, COLOR_CYAN, -1);
    init_pair(8, COLOR_WHITE, -1);

    const char *env_border_color = getenv("HIGHLIGHT_COLOR");
    char current_border_color[16] = "cyan";
    if (env_border_color != NULL) {
        strncpy(current_border_color, env_border_color, sizeof(current_border_color) - 1);
        current_border_color[sizeof(current_border_color) - 1] = '\0';
    }

    const char *env_tree_color = getenv("TREE_COLOR");
    char current_tree_color[16] = "green";
    if (env_tree_color != NULL) {
        strncpy(current_tree_color, env_tree_color, sizeof(current_tree_color) - 1);
        current_tree_color[sizeof(current_tree_color) - 1] = '\0';
    }
    int border_col = COLOR_CYAN;
    if (strcmp(current_border_color, "blue") == 0) border_col = COLOR_BLUE;
    else if (strcmp(current_border_color, "green") == 0) border_col = COLOR_GREEN;
    else if (strcmp(current_border_color, "red") == 0) border_col = COLOR_RED;
    else if (strcmp(current_border_color, "magenta") == 0) border_col = COLOR_MAGENTA;
    else if (strcmp(current_border_color, "yellow") == 0) border_col = COLOR_YELLOW;
    else border_col = COLOR_CYAN;
    init_pair(BORDER_PAIR, border_col, -1);
    int tree_col = COLOR_GREEN;
    if (strcmp(current_tree_color, "blue") == 0) tree_col = COLOR_BLUE;
    else if (strcmp(current_tree_color, "green") == 0) tree_col = COLOR_GREEN;
    else if (strcmp(current_tree_color, "red") == 0) tree_col = COLOR_RED;
    else if (strcmp(current_tree_color, "magenta") == 0) tree_col = COLOR_MAGENTA;
    else if (strcmp(current_tree_color, "yellow") == 0) tree_col = COLOR_YELLOW;
    else tree_col = COLOR_GREEN;
    init_pair(9, tree_col, -1);
    const char *env_branch_palette = getenv("BRANCH_PALETTE");
    char current_branch_palette[16] = "single";
    if (env_branch_palette != NULL) {
        strncpy(current_branch_palette, env_branch_palette, sizeof(current_branch_palette) - 1);
        current_branch_palette[sizeof(current_branch_palette) - 1] = '\0';
    }
    load_branch_palette_from_env();
    
    clear(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);

    refresh();

    int height, width;
    getmaxyx(stdscr, height, width);

    int divider_x = width / 2;
    bool resizing = false;
    MEVENT mevent;

    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    printf("\033[?1002h\033[?1006h"); fflush(stdout);

    WINDOW *left_win = newwin(height, divider_x, 0, 0);
    WINDOW *right_win = newwin(height, width - divider_x, 0, divider_x);
    
    int num_lines = 0;
    LogLine *loglines = parse_log_file(git_log_filepath, &num_lines);
    if (loglines == NULL || num_lines == 0) { endwin(); return 1; }

    char **lines = (char **)malloc(sizeof(char*) * num_lines);
    for (int i = 0; i < num_lines; ++i) {
        lines[i] = loglines[i].line_content;
    }

    int top_line = 0, highlight_line = 0;
    int right_highlight = 0;
    int active_window = 0;
    int ch;
    int exit_code = 0;
    ScreenState current_screen = MAIN_SCREEN;
    
    char current_line_style[20] = "ascii";
    const char* env_line_style = getenv("LINE_STYLE");
    if (env_line_style != NULL) {
        strncpy(current_line_style, env_line_style, sizeof(current_line_style) - 1);
        current_line_style[sizeof(current_line_style) - 1] = '\0';
    }

    while (1) {
        print_left_panel(left_win, lines, num_lines, top_line, (active_window == 0) ? highlight_line : -1, height, divider_x, current_line_style, current_tree_color, current_branch_palette);
        if (current_screen == MAIN_SCREEN) {
            print_right_panel(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x);
        } else if (current_screen == CUSTOMIZE_SCREEN) {
            print_customize_menu(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x, current_line_style, current_border_color, current_tree_color, current_branch_palette);
        } else if (current_screen == PALETTE_EDIT_SCREEN) {
            print_palette_editor(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x);
        }

        if (active_window == 0) {
            wattron(left_win, COLOR_PAIR(BORDER_PAIR) | A_BOLD);
            draw_border(left_win);
            wattroff(left_win, COLOR_PAIR(BORDER_PAIR) | A_BOLD);
            wattron(right_win, A_DIM);
            draw_border(right_win);
            wattroff(right_win, A_DIM);
        } else {
            wattron(right_win, COLOR_PAIR(BORDER_PAIR) | A_BOLD);
            draw_border(right_win);
            wattroff(right_win, COLOR_PAIR(BORDER_PAIR) | A_BOLD);
            wattron(left_win, A_DIM);
            draw_border(left_win);
            wattroff(left_win, A_DIM);
        }

        wnoutrefresh(left_win);
        wnoutrefresh(right_win);
        doupdate();
        
        if (resizing) timeout(20); else timeout(-1);
        ch = getch();
        switch (ch) {
            case KEY_MOUSE: {
                if (getmouse(&mevent) == OK) {
                    int mx = mevent.x;
                    const int DRAG_THRESHOLD = 2;

                    if (mevent.bstate & BUTTON1_PRESSED) {
                        if (abs(mx - divider_x) <= DRAG_THRESHOLD) {
                            resizing = true;
                        }
                    }

                    if (resizing) {
                        int new_div = mx;
                        if (new_div < 10) new_div = 10;
                        if (new_div > width - 10) new_div = width - 10;
                        if (new_div != divider_x) {
                            divider_x = new_div;
                            delwin(left_win);
                            delwin(right_win);
                            left_win = newwin(height, divider_x, 0, 0);
                            right_win = newwin(height, width - divider_x, 0, divider_x);
                            keypad(left_win, TRUE);
                            keypad(right_win, TRUE);
                            touchwin(left_win);
                            touchwin(right_win);
                            print_left_panel(left_win, lines, num_lines, top_line, (active_window == 0) ? highlight_line : -1, height, divider_x, current_line_style, current_tree_color, current_branch_palette);
                            if (current_screen == MAIN_SCREEN) print_right_panel(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x);
                            else if (current_screen == CUSTOMIZE_SCREEN) print_customize_menu(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x, current_line_style, current_border_color, current_tree_color, current_branch_palette);
                            else if (current_screen == PALETTE_EDIT_SCREEN) print_palette_editor(right_win, (active_window == 1) ? right_highlight : -1, height, width - divider_x);
                            wnoutrefresh(left_win);
                            wnoutrefresh(right_win);
                            doupdate();
                        }
                    }

                    if (mevent.bstate & BUTTON1_RELEASED) {
                        resizing = false;
                        timeout(-1);
                    }
                }
            }
            break;
            case KEY_UP: case 'k':
                if (active_window == 0) {
                    if (highlight_line > 0) highlight_line--;
                    if (highlight_line < top_line) top_line = highlight_line;
                } else {
                    if (current_screen != PALETTE_EDIT_SCREEN) {
                        if (right_highlight > 0) right_highlight--;
                    }
                }
                break;
            case KEY_DOWN: case 'j':
                if (active_window == 0) {
                    if (highlight_line < num_lines - 1) highlight_line++;
                    if (highlight_line >= top_line + height - 2) top_line = highlight_line - (height - 3);
                } else {
                    if (current_screen != PALETTE_EDIT_SCREEN) {
                        int max_menu = (current_screen == MAIN_SCREEN) ? 2 : 5;
                        if (right_highlight < max_menu) right_highlight++;
                    }
                }
                break;
            case KEY_LEFT: case 'h':
                active_window = 0;
                break;
            case KEY_RIGHT: case 'l':
                active_window = 1;
                break;
            case '\n':
                if (active_window == 1) {
                    if (current_screen == MAIN_SCREEN) {
                        if (right_highlight == 0) {
                            current_screen = CUSTOMIZE_SCREEN;
                            right_highlight = 0;
                        } else if (right_highlight == 1) {
                            exit_code = 2; goto end_loop;
                        } else if (right_highlight == 2) {
                            exit_code = 3; goto end_loop;
                        }
                    } else if (current_screen == CUSTOMIZE_SCREEN) {
                        if (right_highlight == 0) {
                            const char *styles[] = {"ascii","unicode","unicode-double","unicode-rounded"};
                            int ns = sizeof(styles) / sizeof(styles[0]);
                            int idx = 0;
                            for (int si = 0; si < ns; ++si) if (strcmp(current_line_style, styles[si]) == 0) { idx = si; break; }
                            int next = (idx + 1) % ns;
                            strncpy(current_line_style, styles[next], sizeof(current_line_style) - 1);
                            current_line_style[sizeof(current_line_style) - 1] = '\0';
                            save_setting(project_root, "LINE_STYLE", current_line_style);
                        } else if (right_highlight == 1) {
                            const char *color_options[] = {"cyan","blue","green","red","magenta","yellow"};
                            int num_colors = sizeof(color_options) / sizeof(color_options[0]);
                            int idx = 0;
                            for (int ci = 0; ci < num_colors; ++ci) if (strcmp(current_border_color, color_options[ci]) == 0) { idx = ci; break; }
                            int next = (idx + 1) % num_colors;
                            strncpy(current_border_color, color_options[next], sizeof(current_border_color) - 1);
                            current_border_color[sizeof(current_border_color) - 1] = '\0';
                            save_setting(project_root, "HIGHLIGHT_COLOR", current_border_color);
                            int border_col = COLOR_CYAN;
                            if (strcmp(current_border_color, "blue") == 0) border_col = COLOR_BLUE;
                            else if (strcmp(current_border_color, "green") == 0) border_col = COLOR_GREEN;
                            else if (strcmp(current_border_color, "red") == 0) border_col = COLOR_RED;
                            else if (strcmp(current_border_color, "magenta") == 0) border_col = COLOR_MAGENTA;
                            else if (strcmp(current_border_color, "yellow") == 0) border_col = COLOR_YELLOW;
                            else border_col = COLOR_CYAN;
                            init_pair(BORDER_PAIR, border_col, -1);
                        } else if (right_highlight == 2) {
                            const char *color_options[] = {"cyan","blue","green","red","magenta","yellow"};
                            int num_colors = sizeof(color_options) / sizeof(color_options[0]);
                            int idx = 0;
                            for (int ci = 0; ci < num_colors; ++ci) if (strcmp(current_tree_color, color_options[ci]) == 0) { idx = ci; break; }
                            int next = (idx + 1) % num_colors;
                            strncpy(current_tree_color, color_options[next], sizeof(current_tree_color) - 1);
                            current_tree_color[sizeof(current_tree_color) - 1] = '\0';
                            save_setting(project_root, "TREE_COLOR", current_tree_color);
                            int tree_col = COLOR_GREEN;
                            if (strcmp(current_tree_color, "blue") == 0) tree_col = COLOR_BLUE;
                            else if (strcmp(current_tree_color, "green") == 0) tree_col = COLOR_GREEN;
                            else if (strcmp(current_tree_color, "red") == 0) tree_col = COLOR_RED;
                            else if (strcmp(current_tree_color, "magenta") == 0) tree_col = COLOR_MAGENTA;
                            else if (strcmp(current_tree_color, "yellow") == 0) tree_col = COLOR_YELLOW;
                            else tree_col = COLOR_GREEN;
                            init_pair(9, tree_col, -1);
                            update_branch_color_map(current_branch_palette, current_tree_color);
                        } else if (right_highlight == 3) {
                            const char *palette_options[] = {"single","rainbow","alternate"};
                            int np = sizeof(palette_options) / sizeof(palette_options[0]);
                            int pidx = 0;
                            for (int pi = 0; pi < np; ++pi) if (strcmp(current_branch_palette, palette_options[pi]) == 0) { pidx = pi; break; }
                            int pnext = (pidx + 1) % np;
                            strncpy(current_branch_palette, palette_options[pnext], sizeof(current_branch_palette) - 1);
                                current_branch_palette[sizeof(current_branch_palette) - 1] = '\0';
                                    save_setting(project_root, "BRANCH_PALETTE", current_branch_palette);
                            update_branch_color_map(current_branch_palette, current_tree_color);
                        } else if (right_highlight == 4) {
                            current_screen = PALETTE_EDIT_SCREEN;
                            right_highlight = 0;
                                } else if (right_highlight == 5) { current_screen = MAIN_SCREEN; right_highlight = 0; }
                    } else {
                        /* other screens (like PALETTE_EDIT_SCREEN) handle Enter elsewhere */
                    }
                }
                break;
            case 'q':
                exit_code = 0; goto end_loop;
            default:
                break;
        }

        if (current_screen == PALETTE_EDIT_SCREEN) {
            if (ch == KEY_UP || ch == 'k') {
                if (right_highlight > 0) right_highlight--; else right_highlight = branch_palette_len;
            } else if (ch == KEY_DOWN || ch == 'j') {
                if (right_highlight < branch_palette_len) right_highlight++; else right_highlight = 0;
            } else if (ch == '\n') {
                if (right_highlight == branch_palette_len) {
                    current_screen = CUSTOMIZE_SCREEN; right_highlight = 4;
                    save_branch_palette_to_config(project_root);
                } else {
                    const char *color_options[] = {"cyan","blue","green","red","magenta","yellow","white","black"};
                    int num_colors = sizeof(color_options) / sizeof(color_options[0]);
                    int idx = 0;
                    for (int ci = 0; ci < num_colors; ++ci) if (strcmp(branch_palette_colors[right_highlight], color_options[ci]) == 0) { idx = ci; break; }
                    int next = (idx + 1) % num_colors;
                    strncpy(branch_palette_colors[right_highlight], color_options[next], sizeof(branch_palette_colors[right_highlight]) - 1);
                    branch_palette_colors[right_highlight][sizeof(branch_palette_colors[right_highlight]) - 1] = '\0';
                    update_branch_color_map(current_branch_palette, current_tree_color);
                }
            } else if (ch == 'q') {
                current_screen = MAIN_SCREEN; right_highlight = 0;
            }
        }
    }

end_loop:
    /* disable mouse motion reporting before exit to restore normal terminal state */
    printf("\033[?1002l\033[?1006l"); fflush(stdout);
    free_log_lines(loglines, num_lines);
    free(lines);
    endwin();
    return exit_code;
}

int main(int argc, char *argv[]) {
    // if (argc < 3) return 1;
    return start_ui(argv[1], argv[2]);
}