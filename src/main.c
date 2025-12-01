#include <stdio.h>
#include "ui.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *escape_single_quotes(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    size_t extra = 0;
    for (size_t i = 0; i < len; ++i) if (s[i] == '\'') extra += 3;
    char *out = malloc(len + extra + 3);
    if (!out) return NULL;
    char *p = out; *p++ = '\'';
    for (size_t i = 0; i < len; ++i) {
        if (s[i] == '\'') {
            memcpy(p, "'\\''", 4); p += 4; continue;
        }
        *p++ = s[i];
    }
    *p++ = '\''; *p = '\0';
    return out;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <git-log-file> <project-root>\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        const char *project_root = argv[1];
        char tmp_template[] = "/tmp/gitscope_log_XXXXXX";
        int fd = mkstemp(tmp_template);
        if (fd == -1) return 1;
        FILE *out = fdopen(fd, "w");
        if (!out) { close(fd); unlink(tmp_template); return 1; }
        char *esc = escape_single_quotes(project_root);
        if (!esc) { fclose(out); unlink(tmp_template); return 1; }
        size_t cmdlen = strlen("git -C ") + strlen(esc) + strlen(" log --color=always") + 4;
        char *cmd = malloc(cmdlen);
        if (!cmd) { free(esc); fclose(out); unlink(tmp_template); return 1; }
        snprintf(cmd, cmdlen, "git -C %s log --color=always", esc);
        FILE *gp = popen(cmd, "r");
        if (gp) {
            char buf[4096];
            while (fgets(buf, sizeof(buf), gp)) fputs(buf, out);
            pclose(gp);
        }
        free(cmd); free(esc);
        fclose(out);
        int rc = start_ui(tmp_template, project_root);
        unlink(tmp_template);
        return rc;
    }
    return start_ui(argv[1], argv[2]);
}
