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

    int capacity = 20;
    LogLine* lines = (LogLine*)malloc(sizeof(LogLine) * capacity);
    if (lines == NULL) {
        fclose(fp);
        *num_lines = 0;
        return NULL;
    }

    *num_lines = 0;
    char buffer[2048];
    const char* commit_delim = "<COMMIT>";

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

        buffer[strcspn(buffer, "\n")] = 0;
        char* commit_ptr = strstr(buffer, commit_delim);

        if (commit_ptr != NULL) {
            lines[*num_lines].is_commit = 1;

            *commit_ptr = '\0';
            strncpy(lines[*num_lines].decorations, buffer, sizeof(lines[*num_lines].decorations) - 1);
            lines[*num_lines].decorations[sizeof(lines[*num_lines].decorations) - 1] = '\0';

            char* rest = commit_ptr + strlen(commit_delim);
            char* token;

            token = strtok_r(rest, "|", &rest);
            if (token) strncpy(lines[*num_lines].hash, token, sizeof(lines[*num_lines].hash) - 1);
            lines[*num_lines].hash[sizeof(lines[*num_lines].hash) - 1] = '\0';
            
            token = strtok_r(rest, "|", &rest);
            if (token) strncpy(lines[*num_lines].subject, token, sizeof(lines[*num_lines].subject) - 1);
            lines[*num_lines].subject[sizeof(lines[*num_lines].subject) - 1] = '\0';

            token = strtok_r(rest, "|", &rest);
            if (token) strncpy(lines[*num_lines].author, token, sizeof(lines[*num_lines].author) - 1);
            lines[*num_lines].author[sizeof(lines[*num_lines].author) - 1] = '\0';
            
            token = strtok_r(rest, "|", &rest);
            if (token) strncpy(lines[*num_lines].date, token, sizeof(lines[*num_lines].date) - 1);
            lines[*num_lines].date[sizeof(lines[*num_lines].date) - 1] = '\0';

            char temp_content[2048];
            snprintf(temp_content, sizeof(temp_content), "%s %s %s", lines[*num_lines].decorations, lines[*num_lines].hash, lines[*num_lines].subject);
            lines[*num_lines].line_content = strdup(temp_content);

        } else {
            lines[*num_lines].is_commit = 0;
            lines[*num_lines].line_content = strdup(buffer);
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