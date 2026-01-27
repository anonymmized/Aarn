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
