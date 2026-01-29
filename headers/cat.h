#ifndef CAT_H
#define CAT_H

#define MAXLINE 1024

#define CAT_HL 1

int cat_file(const char *filename);
int parse_cat_flags(int argc, char **argv, int *start);

#endif
