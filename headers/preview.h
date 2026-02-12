#ifndef PREVIEW_H
#define PREVIEW_H

#define CLR_RESET "\033[0m"
#define CLR_NORMAL ""
#define CLR_CURSOR "\033[38;5;214m"
#define CLR_MARKED "\033[30;48;5;214m"
#define CLR_CURSOR_MARKED "\033[30;48;5;214m"
#define GAP 2

struct FSState {
    char **f_list;
    int len;
    int index;
    int offset;
    int real;
    int *marked;
    char *cwd;
};

struct UIState {
    int rows;
    int cols;
    int footer_row;
    int width_list;
    int cols_preview;
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
int is_binary(struct AppState *s);
int check_dir(char *filename);
int list(struct AppState *s);
void get_term_size(int *rows, int *cols);
int fs_empty(struct AppState *s);
void clear_preview_area(struct AppState *s);
void print_name_clipped(struct AppState *s);
void input_monitor(struct AppState *s);
void redraw(struct AppState *s);
void draw_statusbar(struct AppState *s);

#endif
