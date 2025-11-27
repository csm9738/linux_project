#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Commit* parse_git_log_file(const char* filepath, int* num_commits) {
    FILE* fp = fopen(filepath, "r");
    if (fp == NULL) {
        *num_commits = 0;
        return NULL;
    }

    int capacity = 10;
    Commit* commits = (Commit*)malloc(sizeof(Commit) * capacity);
    if (commits == NULL) {
        fclose(fp);
        *num_commits = 0;
        return NULL;
    }

    *num_commits = 0;
    char line[2048];

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (*num_commits >= capacity) {
            capacity *= 2;
            Commit* new_commits = (Commit*)realloc(commits, sizeof(Commit) * capacity);
            if (new_commits == NULL) {
                fclose(fp);
                free_commits(commits);
                *num_commits = 0;
                return NULL;
            }
            commits = new_commits;
        }

        char* token;
        char* rest = line;

        token = strtok_r(rest, "|", &rest);
        if (token) strncpy(commits[*num_commits].hash, token, sizeof(commits[*num_commits].hash) - 1);
        commits[*num_commits].hash[sizeof(commits[*num_commits].hash) - 1] = '\0';

        token = strtok_r(rest, "|", &rest);
        if (token) strncpy(commits[*num_commits].author, token, sizeof(commits[*num_commits].author) - 1);
        commits[*num_commits].author[sizeof(commits[*num_commits].author) - 1] = '\0';
        
        token = strtok_r(rest, "|", &rest);
        if (token) strncpy(commits[*num_commits].date, token, sizeof(commits[*num_commits].date) - 1);
        commits[*num_commits].date[sizeof(commits[*num_commits].date) - 1] = '\0';

        token = strtok_r(rest, "\n", &rest);
        if (token) strncpy(commits[*num_commits].subject, token, sizeof(commits[*num_commits].subject) - 1);
        commits[*num_commits].subject[sizeof(commits[*num_commits].subject) - 1] = '\0';
        
        (*num_commits)++;
    }

    fclose(fp);
    return commits;
}

void free_commits(Commit* commits) {
    if (commits) {
        free(commits);
    }
}
