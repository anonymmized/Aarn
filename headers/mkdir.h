#ifndef MKDIR_H
#define MKDIR_H

#include <sys/stat.h>

#define MKDIR_P 1
#define MKDIR_M 2
#define MKDIR_V 4

int create_dir(char *path, int recursive, mode_t perms, int verbose);
int parse_mkdir_flags(int argc, char **argv, int *start);

#endif
