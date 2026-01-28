#ifndef UTILS_H
#define UTILS_H

enum Commands {CMD_exit, CMD_pwd, CMD_ls, CMD_cd, CMD_clear, CMD_cat, CMD_rm, CMD_mkdir};

typedef struct {
    const char *name;
    int id;
} Cmd;

extern Cmd cmds[];

typedef struct {
    int files;
    int dirs;
} Count;

int get_line(char *line, int lim);
char *skip_spaces(char *s);
int dir_exists(const char *path);
int parse_line(char *line, char **argv, int max);
int get_command_id(char *line);
const char *return_last_dir(const char *workin);
Count dir_items(const char *dirname);

#endif
