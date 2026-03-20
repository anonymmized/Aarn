#ifndef UI_H
#define UI_H

#include "./preview.h"

void draw_statusbar(struct AppState *s);
void draw_frame(struct AppState *s);
void clear_preview_area(struct AppState *s);
void get_term_size(int *rows, int *cols);
void print_name_clipped(struct AppState *s);
void draw_file_preview(struct AppState *s, int file_scroll);
void print_line_clipped(const char *s, int max_width);

#endif
