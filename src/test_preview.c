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

int cat_file(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fp");
        return 1;
    }
    char buf[1024];
    while (fgets(buf, sizeof(buf), fp)) fputs(buf, stdout);
    if (ferror(fp)) {
        perror("fgets");
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}

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
        if (c == 'q') {
            return;
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
    char target_name[1024];
    for (int i = 0; i < len; i++) {
        printf("\033[%d;1H", i + 1);
        char *new_s = strrchr(f_list[i], '/') + 1;
        if (i == target) {
            if (chose == 1) {
                if (check_dir(f_list[i]) == 1) {
                    printf(ORANGE "\t%s\n\n" ESC, new_s);
                    strcpy(target_name, f_list[i]);
                } else if (check_dir(f_list[i]) == 2) {
                    printf(ORANGE "\t%s\n\n" ESC, new_s);
                }
            } else {
                printf(ORANGE "\t%s\n\n" ESC, new_s);
            }
        } else {
            printf("\t%s\n\n", new_s);
        }
    }
    if (chose == 1) { 
        FILE *fp = fopen(target_name, "r");
        if (!fp) {
            perror("fopen");
            return;
        }
        char line[1024];
        int row = 1;
        while (fgets(line, sizeof(line), fp) && row < 40) {
            printf("\033[%d;40H", row);
            printf("%s", line);
            row++;
        }
        fclose(fp);
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
    printf("\033[?25l");
    char *f_list[100];
    int items_count = list(".", f_list);
    printf("Current dir: %d\n", items_count);
    enable_raw();
    input_monitor(f_list, items_count, 11);
    
    disable_raw();
    printf("\033[?25h");
    for (int i = 0; i < items_count; i++) {
        free(f_list[i]);
    }
    return 0;
}
