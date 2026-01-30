#include "../headers/cd.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int cmd_cd(const char *path) {
    if (!path || !*path || *path == ' ') path = getenv("HOME");
    if (!path) return 1;

    if (chdir(path) == -1) {
        perror("cd");
        return 1;
    }
    return 0;
}

int parse_cd_flags(int argc, char **argv, int *start) {
    int flags = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][0] != '\0') {
        for (int j = 1; argv[i][j] != '\0'; j++) {
            char c = argv[i][j];
            if (c == 'h') flags |= CD_HL;
            else {
                fprintf(stderr, "cd: unknown option -%c\n", c);
                return -1;
            }
        }
        i++;
    }
    *start = i;
    return flags;
}
