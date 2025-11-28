#ifndef PARSER_H
#define PARSER_H

typedef struct {
    char* line_content;
    int is_commit;
    
    char decorations[512];
    char hash[41];
    char subject[1024];
    char author[256];
    char date[256];
} LogLine;

LogLine* parse_log_file(const char* filepath, int* num_lines);
void free_log_lines(LogLine* lines, int num_lines);

#endif // PARSER_H