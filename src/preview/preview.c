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

void input_monitor(struct AppState *s) {
    s->fs.index = 0;
    s->fs.offset = 0; 
    s->rt.launched = 1;
    s->rt.mode = 0;
    if (!fs_empty(s)) update_current_file_type(s);
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
            if (s->fs.type == FT_DIR) {
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
                update_current_file_type(s);
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
                update_current_file_type(s);
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
                    s->fs.index = s->fs.len - 1;
                    s->fs.offset = s->fs.len - visible;
                    if (s->fs.offset < 0) s->fs.offset = 0;
                }
                if (s->fs.index < s->fs.offset) {
                    s->fs.offset = s->fs.index;
                }
                update_current_file_type(s);
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
                update_current_file_type(s);
                redraw(s);
            }
            if (seq[1] == 'C') {
                s->rt.mode = 0;
                s->rt.last_key = 'R';
                const char *path = s->fs.f_list[s->fs.index];
                if (fs_empty(s)) continue;
                if (s->fs.type == FT_DIR) {
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
                    update_current_file_type(s);
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
                    update_current_file_type(s);
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
    s->ui.preview_st = 0;
    if (s->fs.type == FT_BINARY) {
        printf("\033[%d;%dH", 1, s->ui.cols_preview);
        printf("<BINARY FILE>");
    } else if (s->fs.type == FT_TEXT) {
        draw_file_preview(s);
        s->ui.preview_st = 1;
    }
    draw_statusbar(s);
    fflush(stdout);
}
