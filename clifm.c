#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

#define MAXLINE 1024
#define DIR_LEN 128

#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define ITALIC "\033[3m"
#define ESC    "\033[0m"

enum Commands {CMD_exit, CMD_pwd, CMD_ls, CMD_cd, CMD_clear};
typedef struct {
    const char *name;
    int id;
} Cmd;
static Cmd cmds[] = {
    {"exit", CMD_exit},
    {"pwd", CMD_pwd},
    {"ls", CMD_ls},
    {"cd", CMD_cd},
    {"clear", CMD_clear}
};

char *skip_spaces(char *);

int get_line(char *line, int lim) {
    int c, i = 0;
    while (--lim > 0 && (c = getchar()) != EOF && c != '\n') {
        line[i++] = c;
    }
    if (c == '\n') {
        line[i++] = '\n';
    }
    line[i] = '\0';
    return i;
}

const char *print_workin(void) {
    static char buf[PATH_MAX];

    if (!getcwd(buf, sizeof(buf))) {
        perror("getcwd");
        return NULL;
    }
    return buf;
}

int list_wd(char *dir) {
    char *new_dir = skip_spaces(dir);
    DIR *w_dir = opendir(new_dir);
    if (!w_dir) {
        fprintf(stderr, "Error with opening dir\n");
        return 1;
    }
    int items_count = 0;
    struct dirent *ent;

    while ((ent = readdir(w_dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        items_count += 1;
    }

    rewinddir(w_dir);

    while ((ent = readdir(w_dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        if (items_count < 10) {
            if (ent->d_type == DT_DIR) fprintf(stdout, ITALIC BOLD "%s " ESC, ent->d_name);
            else fprintf(stdout, DIM "%s " ESC, ent->d_name);
        } else {
            if (ent->d_type == DT_DIR) fprintf(stdout, ITALIC BOLD "%s\n" ESC, ent->d_name);
            else fprintf(stdout, DIM "%s\n" ESC, ent->d_name);
        }
    }
    if (items_count < 8) printf("\n");
    closedir(w_dir);
    return 0;
}

char *skip_spaces(char *s) {
    while (*s == ' ' || *s == '\t') s++;                                                 return s;
}

int dir_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}

   
int cmd_cd(const char *path) {
    if (!path || !*path || *path == ' ') path = getenv("HOME");
    if (!path) return 1;

    if (chdir(path) == -1) {
        perror("cd");
        return 1;
    }
    return 0;
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
    for (int i = 0; i < 5; i++) {
        if (strcmp(command, cmds[i].name) == 0) return cmds[i].id;
    }
    return -1;
}

const char *return_last_dir(const char *workin) {
    const char *last = strrchr(workin, '/');
    return (last && *(last + 1)) ? last + 1 : workin;
}

int main() {
    const char *workin = return_last_dir(print_workin());
    printf(BOLD "%s > " ESC, workin);
    const char *current_dir;
    if ((current_dir = print_workin()) == NULL) {
        fprintf(stderr, "Error with opening dir\n");
        return 1;
    }
    char *c_d = (char *)current_dir;
    char line[MAXLINE];
    int len;
    while ((len = get_line(line, MAXLINE)) > 0) {
        char *argv[16];
        int argc = parse_line(line, argv, 16);
        int output_code = get_command_id(line);
        switch (output_code) {
            case CMD_exit:
                fprintf(stdout, "exit in progress...\n");
                return 0;
            case CMD_pwd: { 
                const char *pwd_v;
                if ((pwd_v = print_workin()) != NULL) {
                    fprintf(stdout, "%s\n", pwd_v);
                    break;
                } else {
                    perror("pwd");
                    break;
                }
            }
            case CMD_clear:
                system("clear");
                break;
            case CMD_ls: 
                if (argc >= 2) {
                    char *arg = argv[1];
                    list_wd(arg);
                    break;
                } else {
                    list_wd(".");
                    break;
                }
            case CMD_cd:
                if (argc >= 2) {
                    char *arg = argv[1];
                    cmd_cd(arg);
                    workin = return_last_dir(print_workin());
                    break;
                } else {
                    cmd_cd(" ");
                    break;
                }
        }
        printf(BOLD "%s > " ESC, workin);

    }
    return 0;
}
