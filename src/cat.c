#include "../headers/cat.h"
#include <stdio.h>

int cat_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("fopen"); return 1; }

    char buf[MAXLINE];
    while (fgets(buf, sizeof(buf), fp)) fputs(buf, stdout);

    if (ferror(fp)) {
        perror("fgets");
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}
