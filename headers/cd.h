#ifndef CD_H
#define CD_H

#define CD_HL 1

int cmd_cd(const char *path);
int parse_cd_flags(int argc, char **argv, int *start);

#endif
