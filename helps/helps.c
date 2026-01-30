#include "helps.h"

#include <stdio.h>

void cat_help(void) {
    printf(
            "usage: cat <file>\n"
            "\n"
            "'cat' displays the contents of the specified file\n"
    );
}

void cd_help(void) {
    printf(
            "usage: cd <path>\n"
            "\n"
            "'cd' changes the current working directory to the specified one\n"
    );
}
