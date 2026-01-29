#include "../headers/mkdir.h"
#include "../headers/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int create_dir(char *path, int recursive, mode_t perms) {
    path[strcspn(path, "\r\n")] = '\0';
    if (!recursive) {
        if (perms) {
            if (mkdir(path, perms) != 0) {
                perror("mkdir");
                return 1;
            }
        } else {
            if (mkdir(path, 0755) != 0) {
                perror("mkdir");
                return 1;
            }
        }
        return 0;
    }

    char dir_path[256];
    strcpy(dir_path, path);
    for (char *p = dir_path; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (!dir_exists(dir_path)) {
                if (!perms) {
                    if (mkdir(dir_path, 0755) != 0) {
                        perror("mkdir");
                        return 1;
                    }
                } else {
                    if (mkdir(dir_path, perms) != 0) {
                        perror("mkdir");
                        return 1;
                    }
                }
            }
            *p = '/';
        }
    }
    if (!dir_exists(dir_path)) {
        if (!perms) {
            if (mkdir(dir_path, 0755) != 0) {
                perror("mkdir");
                return 1;
            }
        } else {
            if (mkdir(dir_path, perms) != 0) {
                perror("mkdir");
                return 1;
            }
        }
    } 
    return 0;
}

int parse_mkdir_flags(int argc, char **argv, int *start) {
    int flags = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][0] != '\0') {
        for (int j = 1; argv[i][j] != '\0'; j++) {
            char c = argv[i][j];
            if (c == 'p') flags |= MKDIR_P;
            else if (c == 'm') flags |= MKDIR_M;
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
