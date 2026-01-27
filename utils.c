#include <stdio.h>

int get_line(char *line, int lim) {
    int c, i = 0;
    while (--lim > 0 && (c = getchar()) != EOF && c != '\n') {
        line[i++] = c;
    }
    if (c == '\n') line[i++] = '\n';
    line[i] = '\0';
    return i;
}

    
