#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include "dirs_help.h"

#define MAXFILES 500
#define MAXLINE 1000

int list_current_dir(char arr[MAXFILES][MAXLINE], const char *directory) {
    DIR *dir;
    struct dirent *entry;
    dir = opendir(directory);
    if (!dir) {
        printf("Error with opening or wrong directory\n");
        return -1;
    }
    int i = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (i >= MAXFILES) {
            printf("Too many files (limit: %d)\n", MAXFILES);
            break;
        }
        strncpy(arr[i], entry->d_name, MAXLINE - 1);
        arr[i][MAXLINE - 1] = '\0';
        char full_path[MAXLINE];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, arr[i]);
        normalize_path(full_path);
        parse_command(full_path);
        i++;
    }
    closedir(dir);
    return i;
}

int check_dir(char *s) {
    char *p = s;
    while (*p != '\0') {
        if (*p == '/' || *p == '.') {
            return 0;
        }
        p++;
    }
    return -1;
}

void normalize_path(char *path) {
    if (path == NULL) return;
    char *read = path;
    char *write = path;
    while (*read) {
        if (*read == '/' && *(read + 1) == '/') {
            read++;
        } else {
            *write++ = *read++;
        }
    }
    *write = '\0';
}

