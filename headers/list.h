#ifndef LIST_H
#define LIST_H

#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define ITALIC "\033[3m"
#define ESC    "\033[0m"

#define LS_A 1

int list_wd(char *dir, int hidden);

int parse_ls_flags(int argc, char **argv, int *start);

#endif
