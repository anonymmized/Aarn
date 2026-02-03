#include "../headers/touch.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int touch_file(const char *path) {
    int existed = (access(path, F_OK) == 0);
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("fopen");
        return 1;
    } else {
        if (!existed) {
            printf("file was created\n");
        } 
        else {
            printf("file was overwritten\n");
        }
        fclose(fp);
        return 0;
    }
}
