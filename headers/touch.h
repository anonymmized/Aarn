#ifndef TOUCH_H
#define TOUCH_H

#define TOUCH_ATIME 1
#define TOUCH_MTIME 2

int touch_file(const char *path, int atime, int mtime);
int parse_touch_flags(int argc, char **argv, int *start);

#endif
