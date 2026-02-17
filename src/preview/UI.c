#include "../../headers/UI.h"
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
        switch (s->fs.type) {
            case FT_DIR:
                printf("\033[%d;45H\033[97;100m<DIR>\033[0m", s->ui.footer_row);
                break;
            case FT_TEXT:
                printf("\033[%d;45H\033[97;100m<TXT>\033[0m", s->ui.footer_row);
                break;
            case FT_BINARY:
                printf("\033[%d;45H\033[97;100m<BIN>\033[0m", s->ui.footer_row);
                break;
            case FT_UNKNOWN:
                printf("\033[%d;45H\033[97;100m<UNK>\033[0m", s->ui.footer_row);
                break;
        }
        printf("\033[%d;55H", s->ui.footer_row);
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
