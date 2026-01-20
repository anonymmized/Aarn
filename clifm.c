#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

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

const char *print_workin(void) {
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

char *skip_spaces(char *s) {
    while (*s == ' ' || *s == '\t') s++;                                                 return s;
}


int main() {
    printf("> ");
    char line[MAXLINE];
    int len;
    while ((len = get_line(line, MAXLINE)) > 0) {
        line[strcspn(line, "\r\n")] = '\0';
        char *s = skip_spaces(line);
        char *command = s;
        while (*s && *s != ' ' && *s != '\t' && *s != '\n') s++;

        if (*s) *s++ = '\0';

        char *arg = skip_spaces(s);

        if (strcmp(command, "exit\n") == 0) {
            fprintf(stdout, "exit in progress...\n");
            break;
        } else if (strcmp(command, "pwd\n") == 0) {
            const char *pwd_v;
            if ((pwd_v = print_workin()) != NULL) {
                fprintf(stdout, "%s\n", pwd_v);
            }
        } else if (strcmp(command, "ls") == 0) {

            list_wd(*arg ? arg : ".");
        }
        printf("> ");
    }
    return 0;
}
