#include "../../headers/preview.h"
#include "../../headers/UI.h"
#include "../../headers/FS.h"
#include "../../headers/utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

SortMode g_sort_mode = SORT_NONE;

void swap(void *a, void *b, size_t size) {
    char tmp[size];
    memcpy(tmp, a, size);
    memcpy(a, b, size);
    memcpy(b, tmp, size);
}

int file_cmp_ptr(const void *a, const void *b) {
    const FileEntry *path1 = *(FileEntry **)a;
    const FileEntry *path2 = *(FileEntry **)b;

    const char *name1 = strrchr(path1->path, '/');
    const char *name2 = strrchr(path2->path, '/');

    name1 = name1 ? name1 + 1 : path1->path;
    name2 = name2 ? name2 + 1 : path2->path;
    if (g_sort_mode == SORT_NONE) return strcmp(name1, name2);
    if (path1->type == FT_DIR && path2->type != FT_DIR) return -1;
    if (path1->type != FT_DIR && path2->type == FT_DIR) return 1;

    switch (g_sort_mode) {
        case SORT_NAME_ASC:
            return strcmp(name1, name2);
        case SORT_NAME_DESC:
            return strcmp(name2, name1);
        case SORT_DATE_DESC:
        case SORT_DATE_ASC: {
            struct stat st1, st2;
            if (stat(path1->path, &st1) != 0) return 0;
            if (stat(path2->path, &st2) != 0) return 0;
            if (g_sort_mode == SORT_DATE_DESC) return (st2.st_mtime > st1.st_mtime) - (st2.st_mtime < st1.st_mtime);
            else return (st1.st_mtime > st2.st_mtime) - (st1.st_mtime < st2.st_mtime);
        }
        default:
            return strcmp(name1, name2);
    }
}

void rebuild_view(struct AppState *s) {
    s->fs.view = realloc(s->fs.view, sizeof(FileEntry*) * s->fs.len)
    s->fs.view = malloc(sizeof(FileEntry*) * s->fs.len);
    for (int i = 0; i < s->fs.len; i++) s->fs.view[i] = &s->fs.f_list[i];
    quick_sort(s->fs.view, 0, s->fs.view_len - 1, sizeof(FileEntry*), file_cmp_ptr);
}

void quick_sort(void *base, int left, int right, size_t size, int (*cmp)(const void *, const void *)) {
    char *arr = (char *)base;
    int i = left;
    int j = right;
    char pivot[size];
    memcpy(pivot, arr + ((left + right) / 2) * size, size);
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

void filter_view(struct AppState *s, char *pattern) {
    s->fs.view_len = 0;
    if (pattern[0] == '\0') {
        for (int i = 0; i < s->fs.len; i++) s->fs.view[s->fs.view_len++] = &s->fs.f_list[i];
        return;
    }
    for (int i = 0; i < s->fs.len; i++) {
        FileEntry *entry = &s->fs.f_list[i];
        char *name = strrchr(entry->path, '/');
        name = name ? name + 1 : entry->path;
        if (strstr(name, pattern)) s->fs.view[s->fs.view_len++] = entry;
    }
}

void sort_view(struct AppState *s) {
    if (s->fs.view_len > 1) {
        quick_sort(s->fs.view, 0, s->fs.view_len - 1, sizeof(FileEntry*), file_cmp_ptr);
    }
}

void flush_input() {
    char tmp;
    while (read(STDIN_FILENO, &tmp, 1) > 0);
}

void input_monitor(struct AppState *s) {
    s->fs.index = 0;
    s->fs.offset = 0; 
    s->rt.launched = 1;
    s->rt.mode = 0;
    redraw(s);
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) continue;
        s->rt.last_key = c;
        if (s->rt.mode == 3) {
            if (c == 27) {
                s->rt.mode = 0;
                rebuild_view(s);
                redraw(s);
                continue;
            }
            if (c == '\n') {
                s->rt.mode = 0;
                redraw(s);
                continue;
            }

            if (c == 127 || c == 8) {
                if (s->fs.last_entered > 0) {
                    s->fs.last_entered--;
                    s->fs.enter_search[s->fs.last_entered] = '\0';
                }
                filter_view(s, s->fs.enter_search);
                sort_view(s);
                redraw(s);
                continue;
            }

            if (c >= 32 && c <= 126) {
                if (s->fs.last_entered < 1023) {
                    s->fs.enter_search[s->fs.last_entered++] = c;
                    s->fs.enter_search[s->fs.last_entered] = '\0';
                }
            }
            filter_view(s, s->fs.enter_search);
            sort_view(s);
            redraw(s);
            continue;
        }
        if (s->rt.mode == 1) {
            if (c == 'a') g_sort_mode = SORT_NAME_ASC;
            else if (c == 'A') g_sort_mode = SORT_NAME_DESC;
            else if (c == 'd') g_sort_mode = SORT_DATE_ASC;
            else if (c == 'D') g_sort_mode = SORT_DATE_DESC;
            else if (c == 'q') {
                s->rt.mode = 0;
                redraw(s);
                continue;
            }
            else continue;
            quick_sort(s->fs.view, 0, s->fs.view_len - 1, sizeof(FileEntry*), file_cmp_ptr);
            s->fs.index = 0;
            s->fs.offset = 0;
            redraw(s);
            continue;
        }
        if (c == 's') {
            quick_sort(s->fs.view, 0, s->fs.view_len - 1, sizeof(FileEntry*), file_cmp_ptr);
            redraw(s);
        }
        if (c == '/') {
            s->rt.mode = 1;
            redraw(s);
            continue;
        }
        if (c == ':') {
            s->rt.mode = 3;
            if (s->fs.enter_search) free(s->fs.enter_search);
            s->fs.enter_search = calloc(1024, 1);
            s->fs.last_entered = 0;
            redraw(s);
            continue;
        }
        if (c == '\n') {
            s->rt.last_key = 'E';
            const char *path = s->fs.view[s->fs.index]->path;
            if (fs_empty(s)) continue;
            if (s->fs.view[s->fs.index]->type == FT_DIR) {
                if (chdir(path) == -1) {
                    continue;
                }
                getcwd(s->fs.cwd, PATH_MAX);
                for (int i = 0; i < s->fs.len; i++) {
                    free(s->fs.f_list[i].path);
                }
                s->fs.len = list(s);
                rebuild_view(s);
                if (fs_empty(s)) continue;
                s->fs.index = 0;
                s->fs.offset = 0;
                memset(s->fs.marked, 0, 1024 * sizeof(int));
                s->fs.marked_len = 0;
                redraw(s);
            } else {
                redraw(s);
            }
            continue;
        }
        if (c == 127 || c == 8) {
            s->rt.last_key = 'B';
            s->rt.mode = 0;
            if (strcmp(s->fs.cwd, "/") != 0) {
                chdir("..");
                getcwd(s->fs.cwd, PATH_MAX);

                for (int i = 0; i < s->fs.len; i++) {
                    free(s->fs.f_list[i].path);
                }
                s->fs.len = list(s);
                rebuild_view(s);
                if (fs_empty(s)) continue;
                s->fs.index = 0;
                s->fs.offset = 0;
                memset(s->fs.marked, 0, 1024 * sizeof(int));
                s->fs.marked_len = 0;
                redraw(s);
            }
            continue;
        }
        if (c == 'q') {
            s->rt.last_key = 'Q';
            s->rt.launched = 0;
            return;
        }
        if (c == ' ') {
            s->rt.last_key = 'S';
            s->fs.marked[s->fs.index] ^= 1;
            if (s->fs.marked[s->fs.index]) s->fs.marked_len++;
            else s->fs.marked_len--;
            if (s->fs.marked_len == 0) s->rt.mode = 0;
            else s->rt.mode = 2;
            redraw(s);
            continue;
        }
        if (c == '\x1b') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;
            int visible = s->ui.rows;

            if (seq[1] == 'A') {
                s->rt.last_key = 'U';
                if (s->fs.index > 0) {
                    s->fs.index--;
                } else {
                    s->fs.index = s->fs.view_len - 1;
                    s->fs.offset = s->fs.view_len - visible;
                    if (s->fs.offset < 0) s->fs.offset = 0;
                }
                if (s->fs.index < s->fs.offset) {
                    s->fs.offset = s->fs.index;
                }
                redraw(s);
            }

            if (seq[1] == 'B') {
                s->rt.last_key = 'D';
                if (s->fs.index < s->fs.view_len - 1) {
                    s->fs.index++;
                } else {
                    s->fs.index = 0;
                    s->fs.offset = 0;
                }

                if (s->fs.index >= s->fs.offset + visible)
                    s->fs.offset = s->fs.index - visible + 1;
                redraw(s);
            }
            if (seq[1] == 'C') {
                s->rt.mode = 0;
                s->rt.last_key = 'R';
                const char *path = s->fs.view[s->fs.index]->path;
                if (fs_empty(s)) continue;
                if (s->fs.view[s->fs.index]->type == FT_DIR) {
                    if (chdir(path) == -1) {
                        continue;
                    }
                    getcwd(s->fs.cwd, PATH_MAX);
                    for (int i = 0; i < s->fs.len; i++) {
                        free(s->fs.f_list[i].path);
                    }
                    s->fs.len = list(s);
                    rebuild_view(s);
                    s->fs.index = 0;
                    s->fs.offset = 0;
                    memset(s->fs.marked, 0, 1024 * sizeof(int));
                    s->fs.marked_len = 0;
                    redraw(s);
                } else {
                    redraw(s);
                }
                continue;
            } 
            if (seq[1] == 'D') {
                s->rt.mode = 0;
                s->rt.last_key = 'L';
                if (strcmp(s->fs.cwd, "/") != 0) {
                    chdir("..");
                    getcwd(s->fs.cwd, PATH_MAX);
                    for (int i = 0; i < s->fs.len; i++) {
                        free(s->fs.f_list[i].path);
                    }
                    s->fs.len = list(s);
                    rebuild_view(s);
                    if (fs_empty(s)) continue;
                    s->fs.index = 0;
                    s->fs.offset = 0;
                    memset(s->fs.marked, 0, 1024 * sizeof(int));
                    s->fs.marked_len = 0;
                    redraw(s);
                }
                continue;
            }
        }
    }
    s->rt.launched = 0;
}

void redraw(struct AppState *s) {
    if (s->fs.view_len == 0) {
        s->ui.width_list = s->ui.cols / 3;
        for (int r = 1; r <= s->ui.rows; r++) {
            printf("\033[%d;1H\033[K", r);
        }
        clear_preview_area(s);
        draw_statusbar(s);
        fflush(stdout);
        return;
    }
    if (fs_empty(s)) {
        clear_preview_area(s);
        draw_statusbar(s);
        fflush(stdout);
        return;
    }
    if (s->fs.index >= s->fs.view_len) return;

    s->ui.width_list = s->ui.cols / 3;
    s->ui.cols_preview = s->ui.width_list + GAP + 1;

    for (int r = 1; r <= s->ui.rows; r++) {
        printf("\033[%d;1H\033[%dX", r, s->ui.width_list);
    }

    clear_preview_area(s);

    for (int i = 0; i < s->ui.rows; i++) {
        s->fs.real = s->fs.offset + i;
        if (s->fs.real >= s->fs.view_len) break;

        printf("\033[%d;1H", i + 1);

        if (s->fs.real == s->fs.index && s->fs.marked[s->fs.real]) {
            printf(CLR_CURSOR_MARKED " ");
            print_name_clipped(s);
            printf(CLR_RESET);
        } else if (s->fs.real == s->fs.index) {
            printf(CLR_CURSOR " ");
            print_name_clipped(s);
            printf(CLR_RESET);
        } else if (s->fs.marked[s->fs.real]) {
            printf(CLR_MARKED " ");
            print_name_clipped(s);
            printf(CLR_RESET);
        } else {
            printf(" ");
            print_name_clipped(s);
        }
        printf("\033[%d;%dH", i + 1, s->ui.width_list);
    }
    s->ui.preview_st = 0;
    if (s->fs.view_len == 0) return;
    FileEntry *cur = s->fs.view[s->fs.index];
    FileType cur_type = cur->type;
    if (cur_type == FT_BINARY) {
        printf("\033[%d;%dH", 1, s->ui.cols_preview);
        printf("<BINARY FILE>");
    } else if (cur_type == FT_TEXT) {
        draw_file_preview(s);
        s->ui.preview_st = 1;
    }
    draw_statusbar(s);
    fflush(stdout);
}
