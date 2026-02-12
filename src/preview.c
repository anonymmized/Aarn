#include "../headers/preview.h"
#include "../headers/utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

int fs_empty(struct AppState *s) {
    return s->fs.len == 0;
}

void draw_statusbar(struct AppState *s) {
    printf("\033[%d;1H\033[K", s->ui.footer_row);
    printf("Mode: ");
    switch (s->rt.mode) {
        case 0:
            printf("Normal");
            break;
        case 1:
            printf("Search");
            break;
        case 2:
            printf("\033[%d;1H\033[K", s->ui.footer_row + 1);
            printf("\033[%d;1HMarked [%d]", s->ui.footer_row, s->fs.marked_len);
            break;
    }
    if (s->rt.mode != 2) {
        printf("\033[%d;15HLk: %c", s->ui.footer_row, s->rt.last_key);
        printf("\033[%d;25H%d|%d", s->ui.footer_row, s->fs.index + 1, s->fs.len);
        const char *path = s->fs.f_list[s->fs.index];
        const char *name = strrchr(path, '/');
        name = name ? name + 1 : path;
        char *dot = strrchr(name, '.');
        if (!dot || dot == name) dot = "";
        struct stat st;
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                printf("\033[%d;40H\033[37;40mDIR\033[0m", s->ui.footer_row);
            }
            if (S_ISREG(st.st_mode)) {
                printf("\033[%d;40H\033[37;40m%s\033[0m", s->ui.footer_row, dot);
            }
        }
        printf("\033[%d;1H\033[K", s->ui.rows + 2);
        printf("%s", s->fs.cwd);
    }
}

int is_binary(struct AppState *s) {
    FILE *fp = fopen(s->fs.f_list[s->fs.index], "rb");
    if (!fp) return 1;
    unsigned char buf[512];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == 0) return 1;
    }
    const char *ext = strrchr(s->fs.f_list[s->fs.index], '.');
    if (!ext) return 0;

    if (!strcmp(ext, ".pdf")) return 1;
    if (!strcmp(ext, ".png")) return 1;
    if (!strcmp(ext, ".jpg")) return 1;
    if (!strcmp(ext, ".jpeg")) return 1;
    if (!strcmp(ext, ".zip")) return 1;
    if (!strcmp(ext, ".tar")) return 1;
    if (!strcmp(ext, ".gz")) return 1;
    if (!strcmp(ext, ".mp4")) return 1;
    if (!strcmp(ext, ".mp3")) return 1;

    return 0;
}

void clear_preview_area(struct AppState *s) {
    for (int r = 1; r <= s->ui.rows; r++) {
        printf("\033[%d;%dH\033[K", r, s->ui.cols_preview);
    }
}


void get_term_size(int *rows, int *cols) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *rows = w.ws_row;
    *cols = w.ws_col;
}

void print_name_clipped(struct AppState *s) {
    int i = 0;
    char *name = strrchr(s->fs.f_list[s->fs.real], '/');
    name = name ? name + 1 : s->fs.f_list[s->fs.real];
    while (i < s->ui.width_list - 2 && name[i] && name[i] != '\n') {
        putchar(name[i]);
        i++;
    }
    if (name[i] != '\0') {
        printf("…");
    }
}
void draw_file_preview(struct AppState *s) {
    if (is_binary(s)) {
        printf("\033[%d;%dH", 1, s->ui.cols_preview);
        printf("<BINARY FILE>");
        return;
    }
    FILE *fp = fopen(s->fs.f_list[s->fs.index], "r");
    if (!fp) return;
    int preview_width = s->ui.cols - s->ui.cols_preview;
    char line[4096];
    int row = 1;
    printf("\033[?7l");
    while (fgets(line, sizeof(line), fp) && row <= s->ui.rows) {
        printf("\033[%d;%dH", row, s->ui.cols_preview);
        print_line_clipped(line, preview_width);
        printf("\033[K");
        row++;
    }
    printf("\033[?7h");
    fclose(fp);
}


void print_line_clipped(const char *s, int max_width) {
    for (int i = 0; i < max_width && s[i] && s[i] != '\n'; i++) {
        putchar(s[i]);
    }
}

int check_dir(const char *filename) {
    struct stat buf;
    if (stat(filename, &buf) != 0) {
        return -1;
    }
    if (S_ISREG(buf.st_mode)) {
        return 1; // file
    } else if (S_ISDIR(buf.st_mode)) {
        return 2; // dir
    } else {                                
        return -1; // not dir & not file
    }
}

void input_monitor(struct AppState *s) {
    s->fs.index = 0;
    s->fs.offset = 0; 
    s->rt.launched = 1;
    s->rt.mode = 0;
    redraw(s);
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {   
            continue;
        }
        s->rt.last_key = c;
        if (c == '\n') {
            s->rt.last_key = 'E';
            const char *path = s->fs.f_list[s->fs.index];
            if (fs_empty(s)) continue;
            if (check_dir(path) == 2) {
                if (chdir(path) == -1) {
                    continue;
                }
                getcwd(s->fs.cwd, PATH_MAX);
                for (int i = 0; i < s->fs.len; i++) {
                    free(s->fs.f_list[i]);
                }
                s->fs.len = list(s);
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
                    free(s->fs.f_list[i]);
                }
                s->fs.len = list(s);
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
            s->rt.mode = 2;
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
                    s->fs.index = s->fs.len - 1;
                    s->fs.offset = s->fs.len - visible;
                    if (s->fs.offset < 0) s->fs.offset = 0;
                }
                if (s->fs.index < s->fs.offset) {
                    s->fs.offset = s->fs.index;
                }
                redraw(s);
            }

            if (seq[1] == 'B') {
                s->rt.last_key = 'D';
                if (s->fs.index < s->fs.len - 1) {
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
                const char *path = s->fs.f_list[s->fs.index];
                if (fs_empty(s)) continue;
                if (check_dir(path) == 2) {
                    if (chdir(path) == -1) {
                        continue;
                    }
                    getcwd(s->fs.cwd, PATH_MAX);
                    for (int i = 0; i < s->fs.len; i++) {
                        free(s->fs.f_list[i]);
                    }
                    s->fs.len = list(s);
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
                        free(s->fs.f_list[i]);
                    }
                    s->fs.len = list(s);
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
    if (fs_empty(s)) {
        clear_preview_area(s);
        draw_statusbar(s);
        fflush(stdout);
        return;
    }
    if (s->fs.index >= s->fs.len) return;

    s->ui.width_list = s->ui.cols / 3;
    s->ui.cols_preview = s->ui.width_list + GAP + 1;

    for (int r = 1; r <= s->ui.rows; r++) {
        printf("\033[%d;1H\033[%dX", r, s->ui.width_list);
    }

    clear_preview_area(s);

    for (int i = 0; i < s->ui.rows; i++) {
        s->fs.real = s->fs.offset + i;
        if (s->fs.real >= s->fs.len) break;

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

    if (check_dir(s->fs.f_list[s->fs.index]) == 1) {
        draw_file_preview(s);
    }
    draw_statusbar(s);
    fflush(stdout);
}

int list(struct AppState *s) {
    DIR *wdir = opendir(s->fs.cwd);
    if (!wdir) {
        fprintf(stderr, "Error wit opening dir\n");
        return 0;
    }
    int items_count = 0;
    struct dirent *ent;
    while ((ent = readdir(wdir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", s->fs.cwd, ent->d_name);
        s->fs.f_list[items_count] = malloc(strlen(fullpath) + 1);
        strcpy(s->fs.f_list[items_count++], fullpath);
    }
    closedir(wdir);
    return items_count;
}
