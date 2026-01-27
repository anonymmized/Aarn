#include "../headers/list.h"
#include "../headers/utils.h"
#include <dirent.h>
#include <string.h>
#include <stdio.h>

int list_wd(char *dir, int hidden) {
    char *new_dir = skip_spaces(dir);
    DIR *w_dir = opendir(new_dir);
    if (!w_dir) {
        fprintf(stderr, "Error with opening dir\n");
        return 1;
    }

    int items_count = 0;
    struct dirent *ent;

    while ((ent = readdir(w_dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        items_count += 1;
    }

    rewinddir(w_dir);
    
    while ((ent = readdir(w_dir)) != NULL) {
        if (!hidden && ent->d_name[0] == '.') continue;
        if (items_count < 10) {
            if (ent->d_type == DT_DIR) fprintf(stdout, ITALIC BOLD "%s " ESC, ent->d_name);
            else fprintf(stdout, DIM "%s " ESC, ent->d_name);
        } else {
            if (ent->d_type == DT_DIR) fprintf(stdout, ITALIC BOLD "%s\n" ESC, ent->d_name);
            else fprintf(stdout, DIM "%s\n" ESC, ent->d_name);
        }
    }
    if (items_count < 10) putchar('\n');
    closedir(w_dir);
    return 0;
}

int parse_ls_flags(int argc, char **argv, int *start) {
    int flags = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][0] != '\0') {
        for (int j = 1; argv[i][j] != '\0'; j++) {
            char c = argv[i][j];
            if (c == 'a') flags |= LS_A;
            else {
                fprintf(stdout, "ls: unknown option -%c\n", c);
                return -1;
            }
        }
        i++;
    }
    *start = i;
    return flags;
}
