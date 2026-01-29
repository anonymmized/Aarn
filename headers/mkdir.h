#ifndef MKDIR_H
#define MKDIR_H

#define MKDIR_P 1
#define MKDIR_M 2
#define MKDIR_V 4

int create_dir(char *path, int recursive);
int parse_mkdir_flags(int argc, char **argv, int *start);

#endif
