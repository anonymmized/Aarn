#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define CLR_RESET         "\033[0m"
#define CLR_NORMAL        ""
#define CLR_CURSOR        "\033[38;5;214m"
#define CLR_MARKED        "\033[30;48;5;148m"
#define CLR_CURSOR_MARKED "\033[30;48;5;214m"
#define GAP 2

struct AppState {
    int len;
    char **f_list;
};

int is_binary(const char *path);
void clear_preview_area(int rows, int cols);
void get_term_size(int *rows, int *cols);
void print_name_clipped(const char *name, int width);
void draw_file_preview(char *filepath, int rows, int cols);
void print_clipped(const char *s, int max_width);
int check_dir(char *filename);
void enable_raw(void);
void disable_raw(void);
void input_monitor(struct AppState *s);
void redraw(char **f_list, int len, int index, int offset, int *marked);
int list(char *dir, char **f_list);

char cwd[PATH_MAX];
struct termios orig;

int is_binary(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 1;
    unsigned char buf[512];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == 0) return 1;
    }
    const char *ext = strrchr(path, '.');
    if (!ext) return 0;

    if (!strcmp(ext, ".pdf")) return 1;
    if (!strcmp(ext, ".png")) return 1;
    if (!strcmp(ext, ".jpg")) return 1;
    if (!strcmp(ext, ".jpeg")) return 1;
    if (!strcmp(ext, ".zip")) return 1;
    if (!strcmp(ext, ".tar")) return 1;
    if (!strcmp(ext, ".gz")) return 1;
    if (!strcmp(ext, ".mp4")) return 1;
    if (!strcmp(ext, ".mp3")) return 1;

    return 0;
}

void clear_preview_area(int rows, int cols) {
    int list_width = cols / 3;
    int preview_col = list_width + GAP + 1;
    for (int r = 1; r <= rows; r++) {
        printf("\033[%d;%dH\033[K", r, preview_col);
    }
}


void get_term_size(int *rows, int *cols) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *rows = w.ws_row;
    *cols = w.ws_col;
}

void print_name_clipped(const char *name, int width) {
    int i = 0;
    while (i < width - 1 && name[i] && name[i] != '\n') {
        putchar(name[i]);
        i++;
    }
    if (name[i] != '\0') {
        printf("…");
    }
}
void draw_file_preview(char *filepath, int rows, int cols) {
    int list_width = cols / 3;
    int preview_col = list_width + GAP + 1;
    if (is_binary(filepath)) {
        printf("\033[%d;%dH", 1, preview_col);
        printf("<BINARY FILE>");
        return;
    }
    FILE *fp = fopen(filepath, "r");
    if (!fp) return;
    int preview_width = cols - preview_col;
    char line[4096];
    int row = 1;
    printf("\033[?7l");
    while (fgets(line, sizeof(line), fp) && row <= rows) {
        printf("\033[%d;%dH", row, preview_col);
        print_clipped(line, preview_width);
        printf("\033[K");
        row++;
    }
    printf("\033[?7h");
    fclose(fp);
}


void print_clipped(const char *s, int max_width) {
    for (int i = 0; i < max_width && s[i] && s[i] != '\n'; i++) {
        putchar(s[i]);
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
    } else {                                                                                 return -1; // not dir & not file
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

void input_monitor(struct AppState *s) {
    int index = 0;
    int offset = 0; 
    int marked[1024] = {0};
    redraw(s->f_list, s->len, index, offset, marked);
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {                                                    continue;
        }
        if (c == '\n') {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", cwd, strrchr(s->f_list[index], '/') + 1);
            if (check_dir(path) == 2) {
                chdir(path);
                getcwd(cwd, sizeof(cwd));
                for (int i = 0; i < s->len; i++) {
                    free(s->f_list[i]);
                }
                s->len = list(cwd, s->f_list);
                index = 0;
                redraw(s->f_list, s->len, index, offset, marked);
            } else {
                redraw(s->f_list, s->len, index, offset, marked);
            }
            continue;
        }
        if (c == 127 || c == 8) {
            if (strcmp(cwd, "/") != 0) {
                chdir("..");
                getcwd(cwd, sizeof(cwd));

                for (int i = 0; i < s->len; i++) {
                    free(s->f_list[i]);
                }
                s->len = list(cwd, s->f_list);
                index = 0;
                redraw(s->f_list, s->len, index, offset, marked);
            }
            continue;
        }
        if (c == 'q') {
            return;
        }
        if (c == ' ') {
            marked[index] = !marked[index];
            redraw(s->f_list, s->len, index, offset, marked);
            continue;
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
                    index = s->len - 1;
                    offset = s->len - visible;
                    if (offset < 0) offset = 0;
                }
                if (index < offset) {
                    offset = index;
                }
                redraw(s->f_list, s->len, index, offset, marked);
            }

            if (seq[1] == 'B') {
                if (index < s->len - 1) {
                    index++;
                } else {
                    index = 0;
                    offset = 0;
                }

                if (index >= offset + visible)
                    offset = index - visible + 1;
                redraw(s->f_list, s->len, index, offset, marked);
            }
            if (seq[1] == 'C') {
                char path[PATH_MAX];
                snprintf(path, sizeof(path), "%s/%s", cwd, strrchr(s->f_list[index], '/') + 1);
                if (check_dir(path) == 2) {
                    chdir(path);
                    getcwd(cwd, sizeof(cwd));
                    for (int i = 0; i < s->len; i++) {
                        free(s->f_list[i]);
                    }
                    s->len = list(cwd, s->f_list);
                    index = 0;
                    redraw(s->f_list, s->len, index, offset, marked);
                } else {
                    redraw(s->f_list, s->len, index, offset, marked);
                }
            } 
            if (seq[1] == 'D') {
                if (strcmp(cwd, "/") != 0) {
                    chdir("..");
                    getcwd(cwd, sizeof(cwd));
                    for (int i = 0; i < s->len; i++) {
                        free(s->f_list[i]);
                    }
                    s->len = list(cwd, s->f_list);
                    index = 0;
                    redraw(s->f_list, s->len, index, offset, marked);
                }
                continue;
            }
        }
    }
}

void redraw(char **f_list, int len, int index, int offset, int *marked) {
    if (index >= len) return;

    int rows, cols;
    get_term_size(&rows, &cols);

    int list_width = cols / 3;
    int preview_col = list_width + GAP + 1;

    for (int r = 1; r <= rows; r++) {
        printf("\033[%d;1H\033[%dX", r, list_width);
    }

    clear_preview_area(rows, cols);

    for (int i = 0; i < rows; i++) {
        int real = offset + i;
        if (real >= len) break;

        printf("\033[%d;1H", i + 1);

        char *name = strrchr(f_list[real], '/');
        name = name ? name + 1 : f_list[real];
        if (real == index && marked[real]) {
            printf(CLR_CURSOR_MARKED " ");
            print_name_clipped(name, list_width - 2);
            printf(CLR_RESET);
        } else if (real == index) {
            printf(CLR_CURSOR " ");
            print_name_clipped(name, list_width - 2);
            printf(CLR_RESET);
        } else if (marked[real]) {
            printf(CLR_MARKED " ");
            print_name_clipped(name, list_width - 2);
            printf(CLR_RESET);
        } else {
            printf(" ");
            print_name_clipped(name, list_width - 2);
        }
        printf("\033[%d;%dH", i + 1, list_width);
    }

    if (check_dir(f_list[index]) == 1) {
        draw_file_preview(f_list[index], rows, cols);
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
    struct AppState st;
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("\033[?1049h");
    printf("\033[?25l");
    fflush(stdout);
    char *f_list[100];
    getcwd(cwd, sizeof(cwd));
    int items_count = list(cwd, f_list);
    printf("Current dir: %d\n", items_count);
    st.f_list = f_list;
    st.len = items_count;
    enable_raw();
    input_monitor(&st);

    disable_raw();
    printf("\033[?25h");
    printf("\033[?1049l");
    fflush(stdout);
    for (int i = 0; i < items_count; i++) {
        free(f_list[i]);
    }
    return 0;
}
