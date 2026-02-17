#ifndef FS_H
#define FS_H

#include "./preview.h"

int list(struct AppState *s);
int fs_empty(struct AppState *s);
void update_current_file_type(struct AppState *s);
FileType get_file_type(const char *path);

#endif
