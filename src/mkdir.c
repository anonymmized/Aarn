#include "../headers/mkdir.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int create_dir(char *path) {
    path[strcspn(path, "\r\n")] = '\0';
    if (mkdir(path, 0755) != 0) {
        perror("mkdir");
        return 1;
    }
    return 0;
}
