#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "dirs_help.h"

#define MAXFILES 500
#define MAXLINE 1000
#define MAX_PATH 1024
#define MAX_OUTPUT 1024

char files_dirs[MAXFILES][MAXLINE] = {0};

int parse_command(char *filepath) {
    char command[2 * MAX_PATH + 10];
    char output[MAX_OUTPUT];
    FILE *fp;

    filepath[strcspn(filepath, "\n")] = '\0';

    if (strchr(filepath, '\'')) {
        fprintf(stderr, "Error: filename contains single quote (not supported in this example)\n");
        return 1;
    }

    snprintf(command, sizeof(command), "file '%s'", filepath);

    fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
        return 1;
    }

    if (fgets(output, sizeof(output), fp) == NULL) {
        fprintf(stderr, "No output from 'file' command\n");
        pclose(fp);
        return 1;
    }

    pclose(fp);

    char *colon = strchr(output, ':');
    if (colon != NULL && *(colon + 1) == ' ') {
        char *filetype = colon + 2;
        filetype[strcspn(filetype, "\n")] = '\0';
        if (strstr(filetype, "directory")) {
            printf("%s → Directory\n", filepath);
        }
        if (strstr(filetype, "text") || strstr(filetype, "ASCII")) {
            printf("%s → Text file\n", filepath);
        } else if (strstr(filetype, "executable")) {
            printf("%s → File executable\n", filepath);
        }
    } else {
        printf("Could not parse output.\n");
    }
    return 0;
}

int m_getline(char *line, int lim) {
    int i = 0;
    int c;
    while (--lim > 0 && (c = getchar()) != EOF && c != '\n') {
        line[i++] = c;
    }
    if (c == '\n') {
        line[i++] = '\n';
    }
    line[i] = '\0';
    return i;
}

int get_dir(const char *input, char *dir, size_t dir_len) {
    const char *p = input;
    if (strncmp(p, "ls", 2) != 0) {
        return -1;
    }

    if (p[2] == '\0' || p[2] == '\n') {
        dir[0] = '.';
        dir[1] = '\0';
        return 0;
    }

    p += 2;
    size_t i = 0;
    while (*p == ' ') p++;
    while (*p != '\0' && i + 1 < dir_len) {
        dir[i] = *p;
        p++;
        i++;
    }
    dir[i] = '\0';
    return 0;
}

int main() {
    char input[MAXLINE];
    char dir[MAXLINE];

    while (1) {
        printf("> ");
        if (fgets(input, sizeof(input), stdin) == NULL) return -1;
        input[strcspn(input, "\n")] = '\0';
        if (get_dir(input, dir, sizeof(dir)) == 0) {
            if (check_dir(dir) != 0) {
                printf("Not directory\n");
                continue;
            } 
            int len = list_current_dir(files_dirs, dir);
        }
        
        printf("\n");
    }
    
    return 0;
}