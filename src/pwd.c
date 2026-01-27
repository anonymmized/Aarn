#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include "../headers/pwd.h"

const char *print_workin(void) {
    static char buf[PATH_MAX];

    if (!getcwd(buf, sizeof(buf))) {
        perror("pwd");
        return NULL;
    }
    return buf;
}

