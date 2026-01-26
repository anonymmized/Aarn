#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#define MAXLINE 1024
#define DIR_LEN 128

#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define ITALIC "\033[3m"
#define ESC    "\033[0m"

#define RM_I 1
#define RM_R 2
#define RM_F 4
#define RM_INTER 8

#define LS_A 1

int remove_dir(const char *name);

enum Commands {CMD_exit, CMD_pwd, CMD_ls, CMD_cd, CMD_clear, CMD_cat, CMD_rm};
typedef struct {
    const char *name;
    int id;
} Cmd;
static Cmd cmds[] = {
    {"exit", CMD_exit},
    {"pwd", CMD_pwd},
    {"ls", CMD_ls},
    {"cd", CMD_cd},
    {"clear", CMD_clear},
    {"cat", CMD_cat},
    {"rm", CMD_rm}
};

typedef struct {
    int files;
    int dirs;
} Count;

char *skip_spaces(char *);

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

int list_wd(char *dir, int hidden) {
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
        if (!hidden && ent->d_name[0] == '.') continue;
        if (items_count < 10) {
            if (ent->d_type == DT_DIR) fprintf(stdout, ITALIC BOLD "%s " ESC, ent->d_name);
            else fprintf(stdout, DIM "%s " ESC, ent->d_name);
        } else {
            if (ent->d_type == DT_DIR) fprintf(stdout, ITALIC BOLD "%s\n" ESC, ent->d_name);
            else fprintf(stdout, DIM "%s\n" ESC, ent->d_name);
        }
    }
    if (items_count < 10) putchar('\n');
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
    for (int i = 0; i < 7; i++) {
        if (strcmp(command, cmds[i].name) == 0) return cmds[i].id;
    }
    return -1;
}

const char *return_last_dir(const char *workin) {
    const char *last = strrchr(workin, '/');
    return (last && *(last + 1)) ? last + 1 : workin;
}

int cat_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("fopen"); return 1; }

    char buf[MAXLINE];
    while (fgets(buf, sizeof(buf), fp)) fputs(buf, stdout);

    if (ferror(fp)) { perror("fgets"); fclose(fp); return 1; }
    fclose(fp);
    return 0;
}

int remove_item(const char *name, int confirm, int recursive, int force) {
    struct stat st;
    if (confirm == 1) {
        fprintf(stdout, "remove %s? [y|n] ", name);
        char ans[10];
        fgets(ans, sizeof(ans), stdin);
        if (ans[0] != 'y' && ans[0] != 'Y') {
            return 0;
        }
    }

    if (lstat(name, &st) != 0) {
        if (force && errno == ENOENT) return 0;
        if (!force) perror("rm");
        return 1;
    }

    if (S_ISDIR(st.st_mode)) {
        if (recursive) {
            if (remove_dir(name) != 0) {
                if (!force) perror("rm");
                return 1;
            }
            return 0;
        }
        if (rmdir(name) != 0) {
            if (!force) perror("rm");
            return 1;
        }
        return 0;
    }

    if (unlink(name) != 0) {
        if (!force) perror("rm");
        return 1;
    }
    return 0;

}

int parse_rm_flags(int argc, char **argv, int *start) {
    int flags = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][0] != '\0') {
        for (int j = 1; argv[i][j] != '\0'; j++) {
            char c = argv[i][j];

            if (c == 'i') flags |= RM_I;
            else if (c == 'r' || c == 'R') flags |= RM_R;
            else if (c == 'f') flags |= RM_F;
            else if (c == 'I') flags |= RM_INTER;
            else {
                fprintf(stderr, "rm: unknown option -%c\n", c);
                return -1;
            }
        }
        i++;
    }
    *start = i;
    return flags;
}

int remove_dir(const char *dirname) {
    DIR *g_dir = opendir(dirname);
    if (!g_dir) {
        fprintf(stderr, "Error with opening dir\n");
        return 1;
    }

    struct dirent *ent;
    while ((ent = readdir(g_dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char fullpath[PATH_MAX];
        if (snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, ent->d_name) >= (int)sizeof(fullpath)) {
            fprintf(stderr, "path too long\n");
            closedir(g_dir);
            return 1;
        }

        struct stat st;
        if (lstat(fullpath, &st) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            remove_dir(fullpath);
            if (rmdir(fullpath) == -1) {
                perror("rmdir");
            }
        } else {
            if (remove(fullpath) != 0) {
                perror("remove");
            }
        }
    }
    closedir(g_dir);

    if (rmdir(dirname) == -1) {
        perror("rmdir");
        return 1;
    }
    return 0;
}

int parse_ls_flags(int argc, char **argv, int *start) {
    int flags = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][0] != '\0') {
        for (int j = 1; argv[i][j] != '\0'; j++) {
            char c = argv[i][j];
            if (c == 'a') flags |= LS_A;
            else {
                fprintf(stderr, "ls: unknown option -%c\n", c);
                return -1;
            }
        }
        i++;
    }
    *start = i;
    return flags;
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
        int output_code = get_command_id(argv[0]);
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
            case CMD_ls: { 
                int start = 1;
                int flags = parse_ls_flags(argc, argv, &start);
                if (flags == -1) break;

                int hidden = (flags & LS_A) != 0;

                for (int i = start; i < argc; i++) {
                    list_wd(argv[i], hidden);
                }
                if (argc <= 2) {
                    list_wd(".", hidden);
                }
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
            case CMD_cat:
                if (argc == 2) {
                    char *filename = argv[1];
                    if (cat_file(filename) == 1) {
                        fprintf(stderr, "Error reading file: %s\n", argv[1]);
                        break;
                    }
                } else {
                    for (int i = 1; i < argc; i++) {
                        if (cat_file(argv[i]) != 0) {
                            fprintf(stderr, "Error reading file: %s\n", argv[i]);
                            break;
                        }
                    } 
                }
                break;
            case CMD_rm: {
                int start = 1;

                int flags = parse_rm_flags(argc, argv, &start);
                if (flags == -1) break;

                int confirm = (flags & RM_I) != 0;
                int recursive = (flags & RM_R) != 0;
                int force = (flags & RM_F) != 0;
                int interactive = (flags & RM_INTER) != 0;
                
                if (force) {
                    confirm = 0;
                    interactive = 0;
                }

                if (start >= argc) {
                    fprintf(stderr, "rm: missing operand\n");
                    break;
                }
                int targets = argc - start;
                if (interactive && (recursive || targets > 3)) {
                    printf("remove: ");
                    for (int i = start; i < argc; i++) {
                        printf("%s ", argv[i]);
                    }
                    printf("[y|n] ");
                    char ans[10];
                    fgets(ans, sizeof(ans), stdin);
                    if (ans[0] != 'y' && ans[0] != 'Y') {
                        break;
                    }
                }

                for (int i = start; i < argc; i++) {
                    remove_item(argv[i], confirm, recursive, force, interactive);
                }
                break;
            }
        }
        printf(BOLD "%s > " ESC, workin);

    }
    return 0;
}
