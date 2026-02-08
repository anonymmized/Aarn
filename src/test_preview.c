#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define ESC    "\033[0m"
#define ORANGE "\033[33m"

void redraw(char **f_list, int len, int target, int offset);
void print_clipped(const char *s, int max_width);
int list(char *dir, char **f_list);

char cwd[PATH_MAX];
struct termios orig;

void get_term_size(int *rows, int *cols) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *rows = w.ws_row;
    *cols = w.ws_col;
}

void draw_file_preview(char *filepath, int rows, int cols) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        perror("fopen");
        return;
    }
    int preview_col = cols / 3; 
    int preview_width = cols - preview_col - 1;

    char line[1024];
    int row = 1;
    printf("\033[?7l");
    while (fgets(line, sizeof(line), fp) && row <= rows) {
        printf("\033[%d;%dH", row, preview_col);
        print_clipped(line, preview_width);
        row++;
    }
    printf("\033[?7h");
    fclose(fp);
}

void print_clipped(const char *s, int max_width) {
    int i = 0;
    while (i < max_width && s[i] && s[i] != '\n') {
        putchar(s[i]);
        i++;
    }
}

int check_dir(char *filename) {
    struct stat buf;
    if (stat(filename, &buf) != 0) {
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

void input_monitor(char **f_list, int len) {
    int index = 0;
    int offset = 0;
    redraw(f_list, len, index, offset);
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            continue;
        }
        if (c == '\n') {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", cwd, strrchr(f_list[index], '/') + 1);
            if (check_dir(path) == 2) {
                chdir(path);
                getcwd(cwd, sizeof(cwd));
                for (int i = 0; i < len; i++) {
                    free(f_list[i]);
                }
                len = list(cwd, f_list);
                index = 0;
                redraw(f_list, len, index, offset);
            } else {
                redraw(f_list, len, index, offset);
            }
            continue;
        }
        if (c == 127 || c == 8) {
            if (strcmp(cwd, "/") != 0) {
                chdir("..");
                getcwd(cwd, sizeof(cwd));

                for (int i = 0; i < len; i++) {
                    free(f_list[i]);
                }
                len = list(cwd, f_list);
                index = 0;
                redraw(f_list, len, index, offset);
            }
            continue;
        }
        if (c == 'q') {
            return;
        }
        if (c == '\x1b') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;
            int rows, cols;
            get_term_size(&rows, &cols);
            int visible = rows;

            if (seq[1] == 'A') {
                if (index > 0) {
                    index--;
                } else {
                    index = len - 1;
                    offset = len - visible;
                    if (offset < 0) offset = 0;
                }
                if (index < offset) {
                    offset = index;
                }
                redraw(f_list, len, index, offset);
            }

            if (seq[1] == 'B') {
                if (index < len - 1) {
                    index++;
                } else {
                    index = 0;
                    offset = 0;
                }

                if (index >= offset + visible) 
                    offset = index - visible + 1;
                redraw(f_list, len, index, offset);
            }
        }
    }
}

void redraw(char **f_list, int len, int index, int offset) {
    printf("\033[H\033[J");
    if (index > len) {
        return;
    }
    int rows, cols;
    get_term_size(&rows, &cols);
    int visible = rows;
    char target_name[1024] = "";
    for (int i = 0; i < visible; i++) {
        int real = offset + i;
        if (real >= len) break;
        printf("\033[%d;1H", i + 1);
        
        char *name = strrchr(f_list[real], '/');
        name = name ? name + 1 : f_list[real];

        if (real == index) {
            printf(ORANGE "\t%s\n" ESC, name);
            if (check_dir(f_list[real]) == 1) {
                strcpy(target_name, f_list[real]);
            }
        } else {
            printf("\t%s\n", name);
        }
    }
    if (target_name[0] != '\0') {
        draw_file_preview(target_name, rows, cols);
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
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("\033[?1049h");
    printf("\033[?25l");
    fflush(stdout);
    char *f_list[100];
    getcwd(cwd, sizeof(cwd));
    int items_count = list(cwd, f_list);
    printf("Current dir: %d\n", items_count);
    enable_raw();
    input_monitor(f_list, items_count);
    
    disable_raw();
    printf("\033[?25h");
    printf("\033[?1049l");
    fflush(stdout);
    for (int i = 0; i < items_count; i++) {
        free(f_list[i]);
    }
    return 0;
}
