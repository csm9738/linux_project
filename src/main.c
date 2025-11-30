#include <stdio.h>
#include "ui.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <git-log-file> <project-root>\n", argv[0]);
        return 1;
    }
    return start_ui(argv[1], argv[2]);
}
