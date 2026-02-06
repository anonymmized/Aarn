#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#define ESC    "\033[0m"
#define ORANGE "\033[33m"

void redraw(char **f_list, int len, int target) {
    if (target > len) {
        return;
    }
    for (int i = 0; i < len; i++) {
        if (i == target - 1) {
            printf(ORANGE "%s\n" ESC, f_list[i]);
        } else {
            printf("%s\n", f_list[i]);
        }
    }
}

int list(char *dir, char **f_list) {
    DIR *wdir = opendir(dir);
    if (!wdir) {
        fprintf(stderr, "Error wit opening dir\n");
        return 0;
    }
    int items_count = 0;
    struct dirent *ent;
    while ((ent = readdir(wdir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        f_list[items_count] = malloc(strlen(ent->d_name) + 1);
        strcpy(f_list[items_count++], ent->d_name);
    }
    closedir(wdir);
    return items_count;
}


int main() {
    char *f_list[100];
    int items_count = list(".", f_list);
    printf("Current dir: %d\n", items_count);
    redraw(f_list, items_count, 11);
    for (int i = 0; i < items_count; i++) {
        free(f_list[i]);
    }
    return 0;
}
