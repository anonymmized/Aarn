#ifndef PREVIEW_H
#define PREVIEW_H

#define CLR_RESET "\033[0m"
#define CLR_NORMAL ""
#define CLR_CURSOR "\033[38;5;214m"
#define CLR_MARKED "\033[30;48;5;214m"
#define CLR_CURSOR_MARKED "\033[30;48;5;214m"
#define GAP 2

typedef enum {
    FT_DIR = 0,
    FT_TEXT,
    FT_BINARY,
    FT_UNKNOWN
} FileType;

struct FSState {
    char **f_list;
    int len;
    int index;
    int offset;
    int real;
    int *marked;
    int marked_len;
    char *cwd;
    FileType type;
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
void print_line_clipped(const char *s, int max_width);
int list(struct AppState *s);
void get_term_size(int *rows, int *cols);
int fs_empty(struct AppState *s);
void clear_preview_area(struct AppState *s);
void print_name_clipped(struct AppState *s);
void input_monitor(struct AppState *s);
void redraw(struct AppState *s);
void draw_statusbar(struct AppState *s, FileType ftype);
FileType get_file_type(const char *path);

#endif
