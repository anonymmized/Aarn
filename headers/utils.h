#ifndef UTILS_H
#define UTILS_H

enum Commands {CMD_exit, CMD_pwd, CMD_ls, CMD_cd, CMD_clear, CMD_cat, CMD_rm, CMD_mkdir, CMD_touch};

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
void enable_raw(void);
void disable_raw(void);
void redraw(const char *buf, const char *workin_dir, int cursor);
char *read_command_line(char **history, int *index, int *history_len, const char *workin_dir);

#endif
