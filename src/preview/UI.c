#include "../../headers/UI.h"
#include "../../headers/FS.h"
#include "../../headers/preview.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define UI_CLR_FRAME "\033[38;5;240m"
#define UI_CLR_TITLE "\033[38;5;222m"
#define UI_CLR_STATUS "\033[38;5;117m"
#define UI_CLR_HELP "\033[38;5;245m"
#define UI_CLR_DIM "\033[38;5;244m"
#define UI_CLR_RESET "\033[0m"

static char status_last_key(char key) {
    return (key >= 32 && key <= 126) ? key : '-';
}

static void clear_inner_line(struct AppState *s, int row) {
    int right = s->ui.cols - 1;
    printf("\033[%d;2H", row);
    for (int i = 0; i < right - 2; i++) putchar(' ');
}

static void print_clipped_text(int row, int col, const char *text, int max_width) {
    if (max_width <= 0) return;

    printf("\033[%d;%dH", row, col);
    int len = (int)strlen(text);
    if (len <= max_width) {
        fputs(text, stdout);
        return;
    }

    if (max_width <= 3) {
        for (int i = 0; i < max_width; i++) putchar('.');
        return;
    }

    for (int i = 0; i < max_width - 3; i++) putchar(text[i]);
    fputs("...", stdout);
}

static void draw_horizontal(int row, int from_col, int to_col, const char *glyph) {
    printf("\033[%d;%dH", row, from_col);
    for (int col = from_col; col <= to_col; col++) {
        fputs(glyph, stdout);
    }
}

static void draw_content_frame(struct AppState *s) {
    int right = s->ui.cols - 1;
    int sep_col = s->ui.cols_preview - 2;
    if (sep_col < 4) sep_col = 4;

    char title[PATH_MAX + 32];
    snprintf(title, sizeof(title), " preview %s ", s->fs.cwd ? s->fs.cwd : "");
    int title_len = (int)strlen(title);
    int max_title = right - 5;
    if (title_len > max_title) title_len = max_title;

    printf(UI_CLR_FRAME "\033[1;1H╭");
    draw_horizontal(1, 2, right - 1, "─");
    printf("\033[1;%dH╮", right);
    printf(UI_CLR_TITLE "\033[1;3H");
    for (int i = 0; i < title_len; i++) putchar(title[i]);
    printf(UI_CLR_FRAME);

    for (int row = s->ui.top_row; row < s->ui.divider_row; row++) {
        printf("\033[%d;1H│\033[%d;%dH│\033[%d;%dH│", row, row, sep_col, row, right);
    }

    printf("\033[%d;1H├", s->ui.divider_row);
    draw_horizontal(s->ui.divider_row, 2, sep_col - 1, "─");
    printf("\033[%d;%dH┼", s->ui.divider_row, sep_col);
    draw_horizontal(s->ui.divider_row, sep_col + 1, right - 1, "─");
    printf("\033[%d;%dH┤", s->ui.divider_row, right);

    printf("\033[%d;1H│\033[%d;%dH│", s->ui.footer_row, s->ui.footer_row, right);
    printf("\033[%d;1H│\033[%d;%dH│", s->ui.help_row, s->ui.help_row, right);

    printf("\033[%d;1H╰", s->ui.frame_bottom);
    draw_horizontal(s->ui.frame_bottom, 2, right - 1, "─");
    printf("\033[%d;%dH╯" UI_CLR_RESET, s->ui.frame_bottom, right);
}

void draw_statusbar(struct AppState *s) {
    clear_inner_line(s, s->ui.footer_row);
    clear_inner_line(s, s->ui.help_row);

    const char *mode_label = "[NORMAL]";
    const char *help_label = "enter open  space mark  : search  a/A name  d/D date  q quit";

    switch (s->rt.mode) {
        case 2:
            mode_label = "[MARKED]";
            help_label = "x delete  m mark-all  u clear  i invert  esc cancel  enter open";
            break;
        case 3:
            mode_label = "[SEARCH]";
            help_label = "type to filter  enter keep  esc clear  backspace delete";
            break;
        case 4:
            mode_label = "[VIEWER]";
            help_label = "up/down scroll  q/esc close";
            break;
        default:
            break;
    }

    printf(UI_CLR_STATUS);
    print_clipped_text(s->ui.footer_row, 3, mode_label, 12);

    char summary[256];
    FileType type = view_empty(s) ? FT_UNKNOWN : s->fs.view[s->fs.index]->type;
    const char *type_label = "UNK";
    switch (type) {
        case FT_DIR:
            type_label = "DIR";
            break;
        case FT_TEXT:
            type_label = "TXT";
            break;
        case FT_BINARY:
            type_label = "BIN";
            break;
        default:
            break;
    }

    snprintf(
        summary,
        sizeof(summary),
        "pos %d/%d  •  type %s  •  marks %d  •  sort %s  •  key %c",
        view_empty(s) ? 0 : s->fs.index + 1,
        s->fs.view_len,
        type_label,
        s->fs.marked_len,
        g_sort_mode == SORT_NAME_ASC  ? "name^" :
        g_sort_mode == SORT_NAME_DESC ? "namev" :
        g_sort_mode == SORT_DATE_ASC  ? "date^" :
        g_sort_mode == SORT_DATE_DESC ? "datev" : "none",
        status_last_key(s->rt.last_key)
    );
    print_clipped_text(s->ui.footer_row, 16, summary, s->ui.cols - 19);
    printf(UI_CLR_HELP);

    if (s->rt.mode == 3) {
        char search_line[1100];
        snprintf(search_line, sizeof(search_line), "search  •  %s", s->fs.enter_search ? s->fs.enter_search : "");
        print_clipped_text(s->ui.help_row, 3, search_line, s->ui.cols - 5);
    } else {
        print_clipped_text(s->ui.help_row, 3, help_label, s->ui.cols - 5);
    }
    printf(UI_CLR_RESET);
}

void clear_preview_area(struct AppState *s) {
    for (int row = s->ui.top_row; row < s->ui.divider_row; row++) {
        printf("\033[%d;%dH", row, s->ui.cols_preview);
        for (int i = s->ui.cols_preview; i < s->ui.cols - 1; i++) putchar(' ');
    }
}

void get_term_size(int *rows, int *cols) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_row == 0 || w.ws_col == 0) {
        *rows = 24;
        *cols = 80;
        return;
    }
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
        fputs("…", stdout);
    }
}

void draw_file_preview(struct AppState *s, int file_scroll) {
    FILE *fp = fopen(s->fs.view[s->fs.index]->path, "r");
    if (!fp) return;

    int preview_width = s->ui.cols - s->ui.cols_preview;
    char line[4096];
    int row = s->ui.top_row;
    int lines_counter = 0;

    printf("\033[?7l");
    while (lines_counter < file_scroll && fgets(line, sizeof(line), fp)) lines_counter++;
    while (fgets(line, sizeof(line), fp) && row < s->ui.divider_row) {
        printf("\033[%d;%dH", row, s->ui.cols_preview);
        print_line_clipped(line, preview_width - 1);
        printf("\033[K");
        row++;
    }
    if (row == s->ui.top_row) {
        printf(UI_CLR_DIM "\033[%d;%dH<empty text preview>" UI_CLR_RESET, s->ui.top_row, s->ui.cols_preview + 1);
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

void draw_frame(struct AppState *s) {
    draw_content_frame(s);
}
