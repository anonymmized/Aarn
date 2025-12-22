#ifndef DIRS_HELP_H
#define DIRS_HELP_H

#define MAXFILES 500
#define MAXLINE 1000

int list_current_dir(char arr[MAXFILES][MAXLINE], const char *directory);
int parse_command(char *filepath);
void normalize_path(char *path);

#endif