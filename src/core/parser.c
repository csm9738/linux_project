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

        if (strstr(buffer, "commit ") != NULL) {
            lines[*num_lines].is_commit = 1;
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