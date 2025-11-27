#ifndef PARSER_H
#define PARSER_H

typedef struct {
    char hash[41];
    char author[256];
    char date[256];
    char subject[1024];
} Commit;

Commit* parse_git_log_file(const char* filepath, int* num_commits);
void free_commits(Commit* commits);

#endif // PARSER_H
