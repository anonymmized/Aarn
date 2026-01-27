#ifndef RM_H
#define RM_H

#define RM_I 1
#define RM_R 2
#define RM_F 4
#define RM_INTER 8
#define RM_V 16

int remove_item(const char *name, int confirm, int recursive, int force, int verbose);

int remove_dir(const char *dirname);

int parse_rm_flags(int argc, char **argv, int *start);

#endif
