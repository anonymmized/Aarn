#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <termios.h>
#include <unistd.h>
#include "../headers/utils.h"
#include <sys/stat.h>

Cmd cmds[] = {
    {"exit", CMD_exit},
    {"pwd", CMD_pwd},
    {"ls", CMD_ls},
    {"cd", CMD_cd},
    {"clear", CMD_clear},
    {"cat", CMD_cat},
    {"rm", CMD_rm},
    {"mkdir", CMD_mkdir}
};

struct termios orig;

char *read_command_line(char **history, int *index, int *history_len, const char *workin_dir) {
    int cursor = 0;
    static char buf[1024];
    int len = 0;

    buf[0] = '\0';
    redraw(buf, workin_dir, cursor);
    while (1) {
        char c;
        read(STDIN_FILENO, &c, 1);

        if (c == '\n') {
            buf[len] = '\0';
            if (len > 0) {
                history[*history_len] = strdup(buf);
                (*history_len)++;
            }
            *index = *history_len;
            printf("\n");
            return buf;
        } else if ((c == 127 || c == 8) && cursor > 0) {
            memmove(buf + cursor - 1, buf + cursor, len - cursor);
            cursor--;
            len--;
            buf[len] = '\0';
            redraw(buf, workin_dir, cursor);
        } else if (c == '\x1b') {
            char seq[2];
            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);
            if (seq[1] == 'A') {
                if (*index > 0) {
                    (*index)--;
                    strcpy(buf, history[*index]);
                    len = cursor = strlen(buf);
                    redraw(buf, workin_dir, cursor);
                }
            }
            if (seq[1] == 'B') {
                if (*index < (*history_len) - 1) {
                    (*index)++;
                    strcpy(buf, history[*index]);
                    len = cursor = strlen(buf);
                } else {
                    *index = *history_len;
                    len = cursor = 0;
                    buf[0] = '\0';
                }
                redraw(buf, workin_dir, cursor);
            }
            if (seq[1] == 'D' && cursor > 0) {
                cursor--;
                redraw(buf, workin_dir, cursor);
            }
            if (seq[1] == 'C' && cursor < len) {
                cursor++;
                redraw(buf, workin_dir, cursor);
            }
        } 
        if (c >= 32 && c < 127 && len < (int)sizeof(buf) - 1) {
            memmove(buf + cursor + 1, buf + cursor, len - cursor);
            buf[cursor] = c;
            cursor++;
            len++;
            buf[len] = '\0';
            redraw(buf, workin_dir, cursor);
        }
    }
}

void enable_raw(void) {
    tcgetattr(STDIN_FILENO, &orig);
    struct termios raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

void redraw(const char *buf, const char *workin_dir, int cursor) {
    int len = strlen(buf);
    printf("\r\033[K%s> ", workin_dir);
    for (int i = 0; i < len; i++) {
        if (i == cursor) 
            printf("\033[7m%c\033[0m", buf[i]);
        else 
            putchar(buf[i]);
    }
    if (cursor == len) {
        printf("\033[7m \033[0m");
    }
    fflush(stdout);
}


int get_line(char *line, int lim) {
    int c, i = 0;
    while (--lim > 0 && (c = getchar()) != EOF && c != '\n') {
        line[i++] = c;
    }
    if (c == '\n') line[i++] = '\n';
    line[i] = '\0';
    return i;
}

char *skip_spaces(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

int dir_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}

int parse_line(char *line, char **argv, int max) {
    int argc = 0;
    line[strcspn(line, "\r\n")] = 0;

    while (*line && argc < max - 1) {
        while (*line == ' ' || *line == '\t') line++;
        if (!*line) break;

        argv[argc++] = line;
        while (*line && *line != ' ' && *line != '\t') line++;
        if (*line) *line++ = 0;
    }

    argv[argc] = NULL;
    return argc;
}

int get_command_id(char *line) {
    char *argv[16];
    int arg_count = parse_line(line, argv, 16);
    if (arg_count < 1) {
        printf("\n");
        return -10;
    }
    char *command = argv[0];
    for (int i = 0; i < 8; i++) {
        if (strcmp(command, cmds[i].name) == 0) return cmds[i].id;
    }
    return -1;
}

const char *return_last_dir(const char *workin) {
    const char *last = strrchr(workin, '/');
    return (last && *(last + 1)) ? last + 1 : workin;
}

Count dir_items(const char *dirname) {
    Count c = {0, 0};
    DIR *w_dir = opendir(dirname);
    if (!w_dir) {
        return c;
    }

    struct dirent *ent;
    while ((ent = readdir(w_dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        if (ent->d_type == DT_DIR) c.dirs++;
        else c.files++;
    }
    closedir(w_dir);
    return c;
}
