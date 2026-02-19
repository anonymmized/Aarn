#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

int file_cmp(const void *a, const void *b);

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

void swap(void *a, void *b, size_t size) {
    char tmp[size];
    memcpy(tmp, a, size);
    memcpy(a, b, size);
    memcpy(b, tmp, size);
}

void quick_sort(void *base, int left, int right, size_t size, int (*cmp)(const void *, const void *)) {
    char *arr = (char *)base;

    int i = left;
    int j = right;
    char pivot[size];

    memcpy(pivot, arr + ((right + left) / 2) * size, size);
    while (i <= j) {
        while (cmp(arr + i * size, pivot) < 0) i++;
        while (cmp(arr + j * size, pivot) > 0) j--;

        if (i <= j) {
            swap(arr + i * size, arr + j * size, size);
            i++;
            j--;
        }
    }
    if (left < j) {
        quick_sort(base, left, j, size, cmp);
    }
    if (right > i) {
        quick_sort(base, i, right, size, cmp);
    }
}

int file_cmp(const void *a, const void *b) {
    const char *path1 = *(const char**)a;
    const char *path2 = *(const char**)b;

    struct stat st1, st2;

    if (stat(path1, &st1) != 0 || stat(path2, &st2) != 0) {
        return 0;
    }
    int dir1 = S_ISDIR(st1.st_mode);
    int dir2 = S_ISDIR(st2.st_mode);

    if (dir1 && !dir2) return -1;
    if (dir2 && !dir1) return 1;

    const char *name1 = strrchr(path1, '/');
    const char *name2 = strrchr(path2, '/');

    name1 = name1 ? name1 + 1 : path1;
    name2 = name2 ? name2 + 1 : path2;

    return strcmp(name1, name2);
}

int main() {
    const char path[] = ".";
    char *f_list[1024];
    int n = list(path, f_list);
    quick_sort(f_list, 0, n-1, sizeof(char *), file_cmp);
    for (int i = 0; i < n; i++) {
        printf("%s\n", f_list[i]);
    }
    return 0;
}
