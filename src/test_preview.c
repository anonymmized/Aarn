#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>

#define ESC    "\033[0m"
#define ORANGE "\033[33m"

void redraw(char **f_list, int len, int target, int chose);

struct termios orig;

int check_dir(char *filename) {
    struct stat buf;
    if (stat(filename, &buf) != 0) {
        perror("stat");
        return -1;
    }
    if (S_ISREG(buf.st_mode)) {
        return 1; // file
    } else if (S_ISDIR(buf.st_mode)) {
        return 2; // dir
    } else {
        return -1; // not dir & not file
    }
}

void enable_raw(void) {
    tcgetattr(STDIN_FILENO, &orig);
    struct termios raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

void input_monitor(char **f_list, int len, int target) {
    int index = 0;
    int chose = 0;
    redraw(f_list, len, index, chose);
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            continue;
        }
        if (c == '\n') {
            chose = 1;
            redraw(f_list, len, index, chose);
            chose = 0;
            continue;
        }
        if (c == '\x1b') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;
            if (seq[1] == 'A') {
                if (index != 0) {
                    index -= 1;
                    redraw(f_list, len, index, chose);
                }
            }
            
            if (seq[1] == 'B') {
                if (index < len) {
                    index += 1;
                    redraw(f_list, len, index, chose);
                }
            }
            
            if (seq[1] == 'C')
                printf("Стрелка вправо\n");
            if (seq[1] == 'D')
                printf("Стрелка влево\n");
             
        }
    }
}

void redraw(char **f_list, int len, int target, int chose) {
    printf("\033[H\033[J");
    if (target > len) {
        return;
    }
    for (int i = 0; i < len; i++) {
        char *new_s = strrchr(f_list[i], '/') + 1;
        if (i == target) {
            if (chose == 1) {
                if (check_dir(f_list[i]) == 1) {
                    printf(ORANGE "%s - file\n" ESC, new_s);
                } else if (check_dir(f_list[i]) == 2) {
                    printf(ORANGE "%s - directory\n" ESC, new_s);
                }
            } else {
                printf(ORANGE "%s\n" ESC, new_s);
            }
        } else {
            printf("%s\n", new_s);
        }
    }
    fflush(stdout);
}

int list(char *dir, char **f_list) {
    DIR *wdir = opendir(dir);
    if (!wdir) {
        fprintf(stderr, "Error wit opening dir\n");
        return 0;
    }
    int items_count = 0;
    struct dirent *ent;
    while ((ent = readdir(wdir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, ent->d_name);
        f_list[items_count] = malloc(strlen(fullpath) + 1);
        strcpy(f_list[items_count++], fullpath);
    }
    closedir(wdir);
    return items_count;
}


int main() {
    char *f_list[100];
    int items_count = list("..", f_list);
    printf("Current dir: %d\n", items_count);
    enable_raw();
    while (1) {
        input_monitor(f_list, items_count, 11);
    }
    disable_raw();
    for (int i = 0; i < items_count; i++) {
        free(f_list[i]);
    }
    return 0;
}
