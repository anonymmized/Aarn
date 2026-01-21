#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAXLINE 1024
#define DIR_LEN 128

#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define ITALIC "\033[3m"
#define ESC    "\033[0m"

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

const char *print_workin() {
    const char *pwd = getenv("PWD");
    if (!pwd) {
        fprintf(stderr, "PWD is not set\n");
        return NULL;
    }
    return pwd;
}

int list_wd(char *dir) {
    char *new_dir = skip_spaces(dir);
    DIR *w_dir = opendir(new_dir);
    if (!w_dir) {
        fprintf(stderr, "Error with opening dir\n");
        return 1;
    }

    struct dirent *ent;
    while ((ent = readdir(w_dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        if (ent->d_type == DT_DIR) {
            printf(BOLD ITALIC "%s\n" ESC, ent->d_name);
        } else {
            printf(DIM "%s\n" ESC, ent->d_name);
        }
    }

    closedir(w_dir);
    return 0;
}

int exists_exact_name_cwd(const char *name) {
    DIR *d = opendir(".");
    if (!d) return 0;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, name) == 0) {
            closedir(d);
            return 1;
        }
    }
    closedir(d);
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
    if (!exists_exact_name_cwd(path) && !strstr(path, "../")) {
        fprintf(stderr, "cd: no such file or directory (case mismatch)\n");
        return 1;
    }
    if (!path || !*path) path = getenv("HOME");
    if (!path) return 1;

    if (chdir(path) == 1) {
        perror("cd");
        return 1;
    }
    return 0;
}


int main() {
    printf("> ");
    const char *current_dir;
    if ((current_dir = print_workin()) == NULL) {
        fprintf(stderr, "Error with opening dir\n");
        return 1;
    }
    char *c_d = (char *)current_dir;
    char line[MAXLINE];
    int len;
    while ((len = get_line(line, MAXLINE)) > 0) {
        line[strcspn(line, "\r\n")] = '\0';
        char *s = skip_spaces(line);
        char *command = s;
        while (*s && *s != ' ' && *s != '\t' && *s != '\n') s++;

        if (*s) *s++ = '\0';

        char *arg = skip_spaces(s);

        if (strcmp(command, "exit") == 0) {
            fprintf(stdout, "exit in progress...\n");
            break;
        } else if (strcmp(command, "pwd") == 0) {
            const char *pwd_v;
            if ((pwd_v = print_workin()) != NULL) {
                fprintf(stdout, "%s\n", pwd_v);
            }
        } else if (strcmp(command, "ls") == 0) {
            list_wd(*arg ? arg : ".");
        } else if (strcmp(command, "cd") == 0) {
            cmd_cd(arg);
        } else if (strcmp(command, "clear") == 0) {
            system("clear");
        }
        printf("> ");
    }
    return 0;
}
