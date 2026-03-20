#ifndef PREVIEW_H
#define PREVIEW_H

#include <stddef.h>

#define CLR_RESET "\033[0m"
#define CLR_NORMAL ""
#define CLR_CURSOR "\033[38;5;214m"
#define CLR_MARKED "\033[30;48;5;214m"
#define CLR_CURSOR_MARKED "\033[30;48;5;214m"
#define GAP 2

typedef enum {
    SORT_NONE,
    SORT_NAME_ASC,
    SORT_NAME_DESC,
    SORT_DATE_DESC,
    SORT_DATE_ASC
} SortMode;

extern SortMode g_sort_mode;

typedef enum {
    FT_DIR = 0,
    FT_TEXT,
    FT_BINARY,
    FT_UNKNOWN
} FileType;

typedef struct {
    char *path;
    FileType type;
    int score;
} FileEntry;

struct FSState {
    FileEntry *f_list;
    FileEntry **view;
    SortMode sort_mode;
    int len;
    int view_len;
    int capacity;
    int index;
    int offset;
    int real;
    int *marked;
    int marked_len;
    char *cwd;
    char *enter_search;
    int last_entered;
};

struct UIState {
    int rows;
    int cols;
    int footer_row;
    int width_list;
    int cols_preview;
    int preview_st;
};

struct RuntimeState {
    int launched;
    char last_key;
    int mode;
};

struct AppState {
    struct FSState fs;
    struct UIState ui;
    struct RuntimeState rt;
};
void rebuild_view(struct AppState *s);
void print_line_clipped(const char *s, int max_width);
int list(struct AppState *s);
void get_term_size(int *rows, int *cols);
int fs_empty(struct AppState *s);
void clear_preview_area(struct AppState *s);
void print_name_clipped(struct AppState *s);
void input_monitor(struct AppState *s);
void redraw(struct AppState *s);
void draw_statusbar(struct AppState *s);
FileType get_file_type(const char *path);
void swap(void *a, void *b, size_t size);
int file_cmp_ptr(const void *a, const void *b);
void quick_sort(void *base, int left, int right, size_t size, int (*cmp)(const void *, const void *));
void sort_view(struct AppState *s, int (*cmp)(const void *, const void *));

#endif
