#include "../headers/cat.h"
#include "../helps/helps.h"
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

int parse_cat_flags(int argc, char **argv, int *start) {
    int flags = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][0] != '\0') {
        for (int j = 1; argv[i][j] != '\0'; j++) {
            char c = argv[i][j];
            if (c == 'h') flags |= CAT_HL;
            else {
                fprintf(stderr, "cat: unknown option -%c\n", c);
                return -1;
            }
        }
        i++;
    }
    *start = i;
    return flags;
}
