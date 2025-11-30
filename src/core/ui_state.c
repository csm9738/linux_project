#include "ui_internal.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>

char column_branch[128][64];
BranchColorMap branch_color_map[256];
int branch_color_map_count = 0;
char branch_palette_colors[16][16];
int branch_palette_len = 0;

void init_default_branch_palette(void) {
    const char *defaults[] = {"red","green","yellow","blue","magenta","cyan"};
    int n = sizeof(defaults)/sizeof(defaults[0]);
    for (int i = 0; i < n; ++i) {
        strncpy(branch_palette_colors[i], defaults[i], sizeof(branch_palette_colors[i]) - 1);
        branch_palette_colors[i][sizeof(branch_palette_colors[i]) - 1] = '\0';
    }
    branch_palette_len = n;
}

void load_branch_palette_from_env(void) {
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

void update_branch_color_map(const char *palette, const char *tree_color) {
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

const char *assign_branch_color(const char *branch_name) {
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

void draw_border(WINDOW *win) { box(win, 0, 0); }

char* expand_tabs(const char* input) {
    int tab_count = 0;
    for (int i = 0; input[i] != '\0'; i++) if (input[i] == '\t') tab_count++;
    char* output = malloc(strlen(input) + tab_count * 7 + 1);
    if (output == NULL) return NULL;
    int i = 0, j = 0;
    while (input[i] != '\0') {
        if (input[i] == '\t') {
            for (int k = 0; k < 8; k++) output[j++] = ' ';
            i++;
        } else output[j++] = input[i++];
    }
    output[j] = '\0';
    return output;
}

char* replace_tree_chars(const char* input, const char* line_style) {
    char* output = malloc(strlen(input) * 4 + 1);
    if (output == NULL) return NULL;
    output[0] = '\0';
    const char* V = "|"; const char* SL = "/"; const char* BS = "\\"; const char* H = "-"; const char* S = "*";
    if (line_style != NULL) {
        if (strcmp(line_style, "unicode") == 0) { V = "│"; SL = "╭"; BS = "╰"; H = "─"; S = "●"; }
        else if (strcmp(line_style, "unicode-double") == 0) { V = "║"; SL = "╔"; BS = "╚"; H = "═"; S = "●"; }
        else if (strcmp(line_style, "unicode-rounded") == 0) { V = "ㅣ"; SL = " r"; BS = "ㄴ"; H = "ㅡ"; S = "*"; }
    }
    for (size_t i = 0; i < strlen(input); i++) {
        char current_char = input[i];
        if (current_char == '|' ) { strcat(output, V); }
        else if (current_char == '/') { strcat(output, SL); }
        else if (current_char == '\\') { strcat(output, BS); }
        else if (current_char == '-') { strcat(output, H); }
        else if (current_char == '*') { strcat(output, S); }
        else { char tmp[2] = {current_char, '\0'}; strcat(output, tmp); }
    }
    return output;
}

char* replace_graph_prefix_preserve_ansi(const char* src, int len, const char* line_style) {
    const char* V = "|"; const char* SL = "/"; const char* BS = "\\"; const char* H = "-"; const char* S = "*";
    if (line_style != NULL) {
        if (strcmp(line_style, "unicode") == 0) { V = "│"; SL = "╭"; BS = "╰"; H = "─"; S = "●"; }
        else if (strcmp(line_style, "unicode-double") == 0) { V = "║"; SL = "╔"; BS = "╚"; H = "═"; S = "●"; }
        else if (strcmp(line_style, "unicode-rounded") == 0) { V = "ㅣ"; SL = " r"; BS = "ㄴ"; H = "ㅡ"; S = "*"; }
    }
    size_t out_cap = len * 4 + 16;
    char* out = malloc(out_cap);
    if (!out) return NULL;
    out[0] = '\0';
    for (int i = 0; i < len; ) {
        unsigned char c = src[i];
        if (c == 0x1b && src[i+1] == '[') {
            int j = i + 2; while (j < len && src[j] != 'm') j++; if (j < len && src[j] == 'm') j++; strncat(out, &src[i], j - i); i = j; continue;
        }
        if (c == '|' ) { strncat(out, V, out_cap - strlen(out) - 1); i++; continue; }
        if (c == '/') { strncat(out, SL, out_cap - strlen(out) - 1); i++; continue; }
        if (c == '\\') { strncat(out, BS, out_cap - strlen(out) - 1); i++; continue; }
        if (c == '-') { strncat(out, H, out_cap - strlen(out) - 1); i++; continue; }
        if (c == '*') { strncat(out, S, out_cap - strlen(out) - 1); i++; continue; }
        strncat(out, (const char*)&src[i], 1); i++;
    }
    return out;
}

int utf8_char_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

int is_commit_line(const char *s) {
    if (!s) return 0;
    int pfx_len = 0;
    while (s[pfx_len] != '\0') {
        if (s[pfx_len] == '\x1b' && s[pfx_len+1] == '[') { pfx_len += 2; while (s[pfx_len] != '\0' && s[pfx_len] != 'm') pfx_len++; if (s[pfx_len] == 'm') pfx_len++; continue; }
        char c = s[pfx_len];
        if (c == ' ' || c == '|' || c == '*' || c == '/' || c == '\\' || c == '+' || c == '-' || c == 'o') { pfx_len++; continue; }
        if (isalnum((unsigned char)c) || c == '(') break;
        break;
    }
    for (int i = 0; i < pfx_len; ++i) {
        unsigned char uc = (unsigned char)s[i];
        if (uc == (unsigned char)'*' || uc == (unsigned char)'o') return 1;
        if (i + 2 < pfx_len) {
            if ((unsigned char)s[i] == 0xE2 && (unsigned char)s[i+1] == 0x97 && (unsigned char)s[i+2] == 0x8F) return 1;
        }
    }
    return 0;
}

int find_next_commit(char **lines, int num_lines, int start_index, int dir) {
    if (!lines || num_lines <= 0) return -1;
    if (start_index < 0) start_index = 0;
    if (start_index >= num_lines) start_index = num_lines - 1;
    if (dir > 0) { for (int i = start_index; i < num_lines; ++i) if (is_commit_line(lines[i])) return i; return -1; }
    else if (dir < 0) { for (int i = start_index; i >= 0; --i) if (is_commit_line(lines[i])) return i; return -1; }
    return -1;
}

char* colorize_graph_prefix_preserve_ansi(const char* src, const char* palette, const char* base_color) {
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
        if (c == 0x1b && i + 1 < n && src[i+1] == '[') { size_t j = i + 2; while (j < n && src[j] != 'm') j++; if (j < n && src[j] == 'm') j++; strncat(out, &src[i], j - i); i = j; continue; }
        if (src[i] == ' ') { strncat(out, " ", out_cap - strlen(out) - 1); i++; continue; }
        const char *color_name = base_color ? base_color : "cyan";
        if (palette && strcmp(palette, "rainbow") == 0) color_name = palette_colors[pos % palette_len];
        else if (palette && strcmp(palette, "alternate") == 0) color_name = palette_colors[(pos % 2) + 1];
        else if (palette && strcmp(palette, "single") == 0) color_name = base_color ? base_color : "cyan";
        int fg = 37;
        if (strcmp(color_name, "black") == 0) fg = 30; else if (strcmp(color_name, "red") == 0) fg = 31; else if (strcmp(color_name, "green") == 0) fg = 32; else if (strcmp(color_name, "yellow") == 0) fg = 33; else if (strcmp(color_name, "blue") == 0) fg = 34; else if (strcmp(color_name, "magenta") == 0) fg = 35; else if (strcmp(color_name, "cyan") == 0) fg = 36; else fg = 37;
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

char* colorize_graph_prefix_with_token_colors(const char* src, const char** token_colors, int token_count) {
    if (!src) return NULL;
    size_t n = strlen(src);
    size_t out_cap = n * 6 + 64;
    char* out = malloc(out_cap);
    if (!out) return NULL;
    out[0] = '\0';
    int pos = 0;
    for (size_t i = 0; i < n; ) {
        unsigned char c = src[i];
        if (c == 0x1b && i + 1 < n && src[i+1] == '[') { size_t j = i + 2; while (j < n && src[j] != 'm') j++; if (j < n && src[j] == 'm') j++; strncat(out, &src[i], j - i); i = j; continue; }
        if (src[i] == ' ') { strncat(out, " ", out_cap - strlen(out) - 1); i++; continue; }
        const char *color_name = (pos < token_count && token_colors && token_colors[pos]) ? token_colors[pos] : "cyan";
        int fg = 37;
        if (strcmp(color_name, "black") == 0) fg = 30; else if (strcmp(color_name, "red") == 0) fg = 31; else if (strcmp(color_name, "green") == 0) fg = 32; else if (strcmp(color_name, "yellow") == 0) fg = 33; else if (strcmp(color_name, "blue") == 0) fg = 34; else if (strcmp(color_name, "magenta") == 0) fg = 35; else if (strcmp(color_name, "cyan") == 0) fg = 36; else fg = 37;
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
            char code_buffer[128]; int j = 0;
            while (line[i] != '\0' && line[i] != 'm' && j < (int)sizeof(code_buffer) - 1) { code_buffer[j++] = line[i++]; }
            code_buffer[j] = '\0';
            if (line[i] != 'm') while (line[i] != '\0' && line[i] != 'm') i++;
            if (j == 0) { wattroff(win, current_attrs); current_attrs = A_NORMAL; if (line[i] == 'm') i++; continue; }
            char *saveptr = NULL;
            char *code = strtok_r(code_buffer, ";", &saveptr);
            while (code != NULL) {
                int value = atoi(code);
                if (value == 0) { attr = A_NORMAL; color_pair = 0; }
                else if (value == 1) { attr |= A_BOLD; }
                else if (value >= 30 && value <= 37) { color_pair = value - 30 + 1; }
                else if (value >= 90 && value <= 97) { color_pair = value - 90 + 1; }
                else if (value == 39) { color_pair = 0; }
                else if (value == 38 || value == 48) {
                    char *next = strtok_r(NULL, ";", &saveptr);
                    if (next != NULL && atoi(next) == 5) {
                        char *param = strtok_r(NULL, ";", &saveptr);
                        if (param != NULL) { int n = atoi(param); color_pair = (n % 8) + 1; }
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
        int start = i; while (line[i] != '\0' && !(line[i] == '\x1b' && line[i+1] == '[')) i++;
        int len = i - start;
        int curx = getcurx(win);
        int available_cols = max_content_x - curx + 1;
        if (available_cols <= 0) break;
        if (len > available_cols) len = available_cols;
        if (len > 0) waddnstr(win, &line[start], len);
    }
    wattroff(win, current_attrs);
}

void save_setting(const char* project_root, const char* key, const char* value) {
    char config_dir_path[1024]; snprintf(config_dir_path, sizeof(config_dir_path), "%s/config", project_root);
    mkdir(config_dir_path, 0755);
    char config_file_path[1024]; snprintf(config_file_path, sizeof(config_file_path), "%s/config/gitscope.conf", project_root);
    char tmp_path[1200]; snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", config_file_path);
    FILE *in = fopen(config_file_path, "r");
    FILE *out = fopen(tmp_path, "w");
    if (out) {
        if (in) {
            char line[2048]; size_t keylen = strlen(key);
            while (fgets(line, sizeof(line), in)) {
                if (strncmp(line, "export ", 7) == 0) {
                    char *eq = strchr(line + 7, '=');
                    if (eq) {
                        size_t namelen = (size_t)(eq - (line + 7));
                        if (namelen == keylen && strncmp(line + 7, key, keylen) == 0) continue;
                    }
                }
                fputs(line, out);
            }
            fclose(in);
        }
        fprintf(out, "export %s=%s\n", key, value);
        fclose(out);
        rename(tmp_path, config_file_path);
    } else {
        FILE* fp = fopen(config_file_path, "w"); if (fp != NULL) { fprintf(fp, "export %s=%s\n", key, value); fclose(fp); }
    }
}

void save_branch_palette_to_config(const char* project_root) {
    char buf[1024]; buf[0] = '\0';
    for (int i = 0; i < branch_palette_len; ++i) {
        if (i) strncat(buf, ",", sizeof(buf) - strlen(buf) - 1);
        strncat(buf, branch_palette_colors[i], sizeof(buf) - strlen(buf) - 1);
    }
    save_setting(project_root, "BRANCH_PALETTE_COLORS", buf);
}
