#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXLINE 1024
#define DIR_LEN 128

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

int main() {
    printf("> ");
    char line[MAXLINE];
    int len;
    while ((len = get_line(line, MAXLINE)) > 0) {
        if (strcmp(line, "exit\n") == 0) {
            fprintf(stdout, "exit in progress...\n");
            break;
        } else if (strcmp(line, "pwd\n") == 0) {
            const char *pwd_v;
            if ((pwd_v = print_workin()) != NULL) {
                fprintf(stdout, "%s\n", pwd_v);
            }
        }
        // printf("%s", line);
        
        printf("> ");
    }
    return 0;
}
