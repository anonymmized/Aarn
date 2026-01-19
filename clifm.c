#include <stdio.h>
#include <string.h>

#define MAXLINE 1024

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

int main() {
    printf("> ");
    char line[MAXLINE];
    int len;
    while ((len = get_line(line, MAXLINE)) > 0) {
        if (strcmp(line, "exit\n") == 0) {
            printf("exit in progress...\n");
            break;
        }
        printf("%s", line);
        
        printf("> ");
    }
    return 0;
}
