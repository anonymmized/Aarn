#include <stdio.h>
#include <string.h>

#define MAXLINE 1000

int get_dir(const char *input, char *dir, size_t dir_len) {
    const char *p = input;
    if (strncmp(p, "ls", 2) != 0) {
        return -1;
    }

    if (p[2] == '\0' || p[2] == '\n') {
        dir[0] = '.';
        dir[1] = '\0';
        return 0;
    }

    p += 2;
    size_t i = 0;
    while (*p == ' ') p++;
    while (*p != '\0' && i + 1 < dir_len) {
        dir[i] = *p;
        p++;
        i++;
    }
    dir[i] = '\0';
    return 0;
}

int main() {
    char input[MAXLINE];
    char dir[MAXLINE] = {0};
    printf("> ");
    if (fgets(input, sizeof(input), stdin) == NULL) return 1;
    get_dir(input, dir, sizeof(dir));
    printf("dir%s", dir);
    return 0;
}