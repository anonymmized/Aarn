#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "../headers/utils.h"
#include <sys/stat.h>

Cmd cmds[] = {
    {"exit", CMD_exit},
    {"pwd", CMD_pwd},
    {"ls", CMD_ls},
    {"cd", CMD_cd},
    {"clear", CMD_clear},
    {"cat", CMD_cat},
    {"rm", CMD_rm}
};

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
    for (int i = 0; i < 7; i++) {
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
