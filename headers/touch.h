#ifndef TOUCH_H
#define TOUCH_H

#define TOUCH_ATIME 1
#define TOUCH_MTIME 2
#define TOUCH_STAMP 4

int atoi_n(const char *s, int pos, int len);
int touch_file(const char *path, char *stamp, int atime, int mtime, int has_stamp);
int parse_touch_flags(int argc, char **argv, int *start);

#endif
