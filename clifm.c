#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "dirs_help.h"

#define MAXFILES 500
#define MAXLINE 1000
#define MAX_PATH 1024
#define MAX_OUTPUT 1024
#define MAXARGS 50

char input_args[MAXARGS][MAXLINE] = {0};
int args_count = 0;
const char *del_type = " ";

void parse_args(char *input) {
    args_count = 0;
    char *token = strtok(input, del_type);
    while (token != NULL) {
        strncpy(input_args[args_count], token, MAXLINE - 1);
        input_args[args_count][MAXLINE - 1] = '\0';
        token = strtok(NULL, del_type);
        args_count++;
    }
}

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
        parse_args(input);
        if (strncmp(input_args[0], "ls", 2) == 0) {
            char *target_dir = (args_count > 1) ? input_args[1] : ".";
            if (check_dir(target_dir) != 0) {
                printf("Not directory\n");
                continue;
            }
            int len = list_current_dir(files_dirs, target_dir);
        }  else if (strncmp(input_args[0], "cd", 2) == 0) {
            printf("You chose 'cd'\n");
            printf("Args: ");
            for (int i = 1; i < args_count; i++) {
                printf("%s ", input_args[i]);
            }
        }
        else {
            printf("Unknown command\n");
        }
        
        printf("\n");
    }
    
    return 0;
}