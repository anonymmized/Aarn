#ifndef FS_H
#define FS_H

#include "./preview.h"

int count_file_lines(const char *path);
int list(struct AppState *s);
int fs_empty(struct AppState *s);
int view_empty(struct AppState *s);
FileType get_file_type(const char *path);

#endif
