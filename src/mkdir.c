#include "../headers/mkdir.h"
#include "../headers/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

static int mkdir_with_mode(const char *path, mode_t perms) {
    mode_t mode = perms ? perms : 0755;
    if (mkdir(path, mode) == 0 || errno == EEXIST) {
        return 0;
    }
    perror("mkdir");
    return 1;
}

int create_dir(char *path, int recursive, mode_t perms, int verbose) {
    path[strcspn(path, "\r\n")] = '\0';
    if (path[0] == '\0') {
        fprintf(stderr, "mkdir: empty path\n");
        return 1;
    }
    if (!recursive) {
        if (mkdir_with_mode(path, perms) != 0) return 1;
        if (verbose) {
            printf("%s\n", path);
        }
        return 0;
    }

    char dir_path[PATH_MAX];
    if (snprintf(dir_path, sizeof(dir_path), "%s", path) >= (int)sizeof(dir_path)) {
        fprintf(stderr, "mkdir: path too long\n");
        return 1;
    }

    for (char *p = dir_path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (!dir_exists(dir_path)) {
                if (mkdir_with_mode(dir_path, perms) != 0) return 1;
                if (verbose) {
                    printf("%s\n", dir_path);
                }
            }
            *p = '/';
        }
    }
    if (!dir_exists(dir_path)) {
        if (mkdir_with_mode(dir_path, perms) != 0) return 1;
        if (verbose) {
            printf("%s\n", dir_path);
        }
    } 
    return 0;
}

int parse_mkdir_flags(int argc, char **argv, int *start) {
    int flags = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][1] != '\0') {
        for (int j = 1; argv[i][j] != '\0'; j++) {
            char c = argv[i][j];
            if (c == 'p') flags |= MKDIR_P;
            else if (c == 'm') flags |= MKDIR_M;
            else if (c == 'v') flags |= MKDIR_V;
            else {
                fprintf(stderr, "mkdir: unknown option -%c\n", c);
                return -1;
            }
        }
        i++;
    }
    *start = i;
    return flags;
}
