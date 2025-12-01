#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LogLine* parse_log_file(const char* filepath, int* num_lines) {
    FILE* fp = fopen(filepath, "r");
    if (fp == NULL) {
        *num_lines = 0;
        return NULL;
    }
    int capacity = 256;
    LogLine* lines = (LogLine*)malloc(sizeof(LogLine) * capacity);
    if (lines == NULL) {
        fclose(fp);
        *num_lines = 0;
        return NULL;
    }

    *num_lines = 0;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (*num_lines >= capacity) {
            capacity *= 2;
            LogLine* new_lines = (LogLine*)realloc(lines, sizeof(LogLine) * capacity);
            if (new_lines == NULL) {
                free_log_lines(lines, *num_lines);
                fclose(fp);
                *num_lines = 0;
                return NULL;
            }
            lines = new_lines;
        }

        buffer[strcspn(buffer, "\n")] = '\0';
        lines[*num_lines].line_content = strdup(buffer);
        lines[*num_lines].is_commit = 0;
        lines[*num_lines].decorations[0] = '\0';
        lines[*num_lines].hash[0] = '\0';
        lines[*num_lines].subject[0] = '\0';
        lines[*num_lines].author[0] = '\0';
        lines[*num_lines].date[0] = '\0';

        char *cmark = strstr(buffer, "commit ");
        if (cmark != NULL) {
            lines[*num_lines].is_commit = 1;
            cmark += 7;
            char hbuf[64]; int hi = 0;
            while (cmark[hi] != '\0' && hi < 40 && ((cmark[hi] >= '0' && cmark[hi] <= '9') || (cmark[hi] >= 'a' && cmark[hi] <= 'f') || (cmark[hi] >= 'A' && cmark[hi] <= 'F'))) {
                hbuf[hi] = cmark[hi]; hi++;
            }
            hbuf[hi] = '\0';
            if (hi > 0) {
                strncpy(lines[*num_lines].hash, hbuf, sizeof(lines[*num_lines].hash)-1);
                lines[*num_lines].hash[sizeof(lines[*num_lines].hash)-1] = '\0';
            }
        }

        (*num_lines)++;
    }

    fclose(fp);
    return lines;
}

void free_log_lines(LogLine* lines, int num_lines) {
    if (lines == NULL) return;
    for (int i = 0; i < num_lines; i++) {
        if (lines[i].line_content) {
            free(lines[i].line_content);
        }
    }
    free(lines);
}