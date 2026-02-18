#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

int list(const char *path, char **f_list) {
    DIR *wdir = opendir(path);
    if (!wdir) {
        fprintf(stderr, "Error with opening dir\n");
        return 0;
    }
    int items_count = 0;
    struct dirent *ent;
    while ((ent = readdir(wdir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, ent->d_name);
        f_list[items_count] = malloc(strlen(fullpath) + 1);
        strcpy(f_list[items_count++], fullpath);
    }
    closedir(wdir);
    return items_count;
}

int comparator(char *path1, char *path2) {
    struct stat st1;
    struct stat st2;
    if (stat(path1, &st1) == 0 && stat(path2, &st2) == 0) {
        if (S_ISDIR(st1.st_mode) && S_ISDIR(st2.st_mode)) {
            char *p1 = path1;
            char *p2 = path2;
            while (*p1 != '\0' || *p2 != '\0') {
                if ((int)*p1 > (int)*p2) return -1;
                else if ((int)*p1 < (int)*p2) return 1;
                else {
                    p1++;
                    p2++;
                }
            }
        } 
        else if (S_ISDIR(st1.st_mode) && !S_ISDIR(st2.st_mode)) return 1;
        else if (!S_ISDIR(st1.st_mode) && S_ISDIR(st2.st_mode)) return -1;
        else {
            char *p1 = path1;
            char *p2 = path2;
            while (*p1 != '\0' || *p2 != '\0') {
                if ((int)*p1 > (int)*p2) return -1;
                else if ((int)*p1 < (int)*p2) return 1;
                else {
                    p1++;
                    p2++;
                }
            }
        }
    }
    return 0;
}

int main() {
    const char *path = ".";
    char **f_list = malloc(sizeof(char *) * 1024);
    int len = list(path, f_list);
    for (int i = 0; i < len - 1; i++) {
        printf("%d - %d - %d\t%s\n", i + 1, comparator(f_list[i], f_list[i+1]), i+2, f_list[i]);
        free(f_list[i]);
    }
    return 0;
}
