#include "../../headers/UI.h"
#include "../../headers/FS.h"
#include "../../headers/preview.h"

#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

void draw_statusbar(struct AppState *s) {
    printf("\033[%d;1H\033[K", s->ui.footer_row);
    printf("Mode: ");
    switch (s->rt.mode) {
        case 0:
            printf("[NORMAL]");
            break;
        case 2:
            printf("\033[%d;1H\033[K", s->ui.footer_row + 1);
            printf("\033[%d;1H[MARKED] [%d]", s->ui.footer_row, s->fs.marked_len);
            break;
        case 3:
            printf("\033[%d;1H\033[K", s->ui.footer_row);
            printf("[SEARCH]");
            printf("\033[%d;1H\033[K", s->ui.footer_row + 1);
            printf("\033[%d;1H: %s", s->ui.footer_row + 1, s->fs.enter_search);
            break;
        case 4:
            printf("\033[%d;1H\033[K", s->ui.footer_row);
            printf("[VIEWER]");
            printf("\033[%d;1H\033[K", s->ui.footer_row + 1);
            break;
    }
    int sort_col = s->ui.cols > 18 ? s->ui.cols - 17 : 1;
    printf("\033[%d;%dH", s->ui.footer_row, sort_col);
    switch (g_sort_mode) {
        case SORT_NAME_ASC:
            printf("sort:name↑");
            break;
        case SORT_NAME_DESC:
            printf("sort:name↓");
            break;
        case SORT_DATE_DESC:
            printf("sort:date↓");
            break;
        case SORT_DATE_ASC:
            printf("sort:date↑");
            break;
        default:
            printf("sort:none");
            break;
    }
    if (s->rt.mode != 2 && s->rt.mode != 3 && s->rt.mode != 4) {
        int info_col = s->ui.cols > 55 ? 15 : 1;
        printf("\033[%d;%dHLk: %c", s->ui.footer_row, info_col, s->rt.last_key ? s->rt.last_key : '-');
        if (!view_empty(s)) {
            int pos_col = s->ui.cols > 45 ? info_col + 10 : info_col + 8;
            printf("\033[%d;%dH%d|%d", s->ui.footer_row, pos_col, s->fs.index + 1, s->fs.view_len);
        }
        if (s->fs.view_len > 0) {
            FileType type = s->fs.view[s->fs.index]->type; 
            int type_col = s->ui.cols > 35 ? s->ui.cols / 2 : 1;
            switch (type) {
                case FT_DIR:
                    printf("\033[%d;%dH\033[97;100m<DIR>\033[0m", s->ui.footer_row, type_col);
                    break;
                case FT_TEXT:
                    printf("\033[%d;%dH\033[97;100m<TXT>\033[0m", s->ui.footer_row, type_col);
                    break;
                case FT_BINARY:
                    printf("\033[%d;%dH\033[97;100m<BIN>\033[0m", s->ui.footer_row, type_col);
                    break;
                case FT_UNKNOWN:
                    printf("\033[%d;%dH\033[97;100m<UNK>\033[0m", s->ui.footer_row, type_col);
                    break;
            }
        }
        int preview_col = s->ui.cols > 28 ? s->ui.cols / 2 + 10 : 1;
        printf("\033[%d;%dH", s->ui.footer_row, preview_col);
        switch(s->ui.preview_st) {
            case 0:
                printf("Prv: empt");
                break;
            case 1:
                printf("Prv: actv");
                break;
        }
        printf("\033[%d;1H\033[K", s->ui.rows + 2);
        printf("%s", s->fs.cwd);
    }
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
    char *name = strrchr(s->fs.view[s->fs.real]->path, '/');
    name = name ? name + 1 : s->fs.view[s->fs.real]->path;
    while (i < s->ui.width_list - 2 && name[i] && name[i] != '\n') {
        unsigned char ch = (unsigned char)name[i];
        putchar((ch >= 32 && ch < 127) ? ch : '?');
        i++;
    }
    if (name[i] != '\0') {
        printf("…");
    }
}

void draw_file_preview(struct AppState *s, int file_scroll) {
    FILE *fp = fopen(s->fs.view[s->fs.index]->path, "r");
    if (!fp) return;
    int preview_width = s->ui.cols - s->ui.cols_preview;
    char line[4096];
    int row = 1;
    int lines_counter = 0;
    printf("\033[?7l");
    while (lines_counter < file_scroll && fgets(line, sizeof(line), fp)) lines_counter += 1;
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
        unsigned char ch = (unsigned char)s[i];
        if (ch == '\t') {
            putchar(' ');
            continue;
        }
        putchar((ch >= 32 && ch < 127) ? ch : '?');
    }
}
