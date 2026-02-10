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

struct FSState {
    char **f_list;
    int len;
    int index;
    int offset;
    int *marked;
    char *cwd;
};

struct UIState {
    int rows;
    int cols;
    int width_list;
    int cols_preview;
};

struct AppState {
    struct FSState fs;
    struct UIState ui;
};

int is_binary(const char *path);
void clear_preview_area(struct AppState *s);
void get_term_size(int *rows, int *cols);
void print_name_clipped(const char *name, struct AppState *s);
void draw_file_preview(char *filepath, int rows, int cols);
void print_clipped(const char *s, int max_width);
int check_dir(char *filename);
void enable_raw(void);
void disable_raw(void);
void input_monitor(struct AppState *s);
void redraw(struct AppState *s);
int list(char *dir, char **f_list);

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

void clear_preview_area(struct AppState *s) {
    for (int r = 1; r <= s->ui.rows; r++) {
        printf("\033[%d;%dH\033[K", r, s->ui.cols_preview);
    }
}


void get_term_size(int *rows, int *cols) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *rows = w.ws_row;
    *cols = w.ws_col;
}

void print_name_clipped(const char *name, struct AppState *s) {
    int i = 0;
    while (i < s->ui.width_list - 2 && name[i] && name[i] != '\n') {
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

void input_monitor(struct AppState *s) {
    s->fs.index = 0;
    s->fs.offset = 0; 
    redraw(s);
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {   
            continue;
        }
        if (c == '\n') {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", s->fs.cwd, strrchr(s->fs.f_list[s->fs.index], '/') + 1);
            if (check_dir(path) == 2) {
                chdir(path);
                getcwd(s->fs.cwd, sizeof(s->fs.cwd));
                for (int i = 0; i < s->fs.len; i++) {
                    free(s->fs.f_list[i]);
                }
                s->fs.len = list(s->fs.cwd, s->fs.f_list);
                s->fs.index = 0;
                redraw(s);
            } else {
                redraw(s);
            }
            continue;
        }
        if (c == 127 || c == 8) {
            if (strcmp(s->fs.cwd, "/") != 0) {
                chdir("..");
                getcwd(s->fs.cwd, PATH_MAX);

                for (int i = 0; i < s->fs.len; i++) {
                    free(s->fs.f_list[i]);
                }
                s->fs.len = list(s->fs.cwd, s->fs.f_list);
                s->fs.index = 0;
                redraw(s);
            }
            continue;
        }
        if (c == 'q') {
            return;
        }
        if (c == ' ') {
            s->fs.marked[s->fs.index] = !s->fs.marked[s->fs.index];
            redraw(s);
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
                if (s->fs.index > 0) {
                    s->fs.index--;
                } else {
                    s->fs.index = s->fs.len - 1;
                    s->fs.offset = s->fs.len - visible;
                    if (s->fs.offset < 0) s->fs.offset = 0;
                }
                if (s->fs.index < s->fs.offset) {
                    s->fs.offset = s->fs.index;
                }
                redraw(s);
            }

            if (seq[1] == 'B') {
                if (s->fs.index < s->fs.len - 1) {
                    s->fs.index++;
                } else {
                    s->fs.index = 0;
                    s->fs.offset = 0;
                }

                if (s->fs.index >= s->fs.offset + visible)
                    s->fs.offset = s->fs.index - visible + 1;
                redraw(s);
            }
            if (seq[1] == 'C') {
                char path[PATH_MAX];
                snprintf(path, sizeof(path), "%s/%s", s->fs.cwd, strrchr(s->fs.f_list[s->fs.index], '/') + 1);
                if (check_dir(path) == 2) {
                    chdir(path);
                    getcwd(s->fs.cwd, PATH_MAX);
                    for (int i = 0; i < s->fs.len; i++) {
                        free(s->fs.f_list[i]);
                    }
                    s->fs.len = list(s->fs.cwd, s->fs.f_list);
                    s->fs.index = 0;
                    redraw(s);
                } else {
                    redraw(s);
                }
            } 
            if (seq[1] == 'D') {
                if (strcmp(s->fs.cwd, "/") != 0) {
                    chdir("..");
                    getcwd(s->fs.cwd, PATH_MAX);
                    for (int i = 0; i < s->fs.len; i++) {
                        free(s->fs.f_list[i]);
                    }
                    s->fs.len = list(s->fs.cwd, s->fs.f_list);
                    s->fs.index = 0;
                    redraw(s);
                }
                continue;
            }
        }
    }
}

void redraw(struct AppState *s) {
    if (s->fs.index >= s->fs.len) return;

    s->ui.width_list = s->ui.cols / 3;
    s->ui.cols_preview = s->ui.width_list + GAP + 1;

    for (int r = 1; r <= s->ui.rows; r++) {
        printf("\033[%d;1H\033[%dX", r, s->ui.width_list);
    }

    clear_preview_area(s);

    for (int i = 0; i < s->ui.rows; i++) {
        int real = s->fs.offset + i;
        if (real >= s->fs.len) break;

        printf("\033[%d;1H", i + 1);

        char *name = strrchr(s->fs.f_list[real], '/');
        name = name ? name + 1 : s->fs.f_list[real];
        if (real == s->fs.index && s->fs.marked[real]) {
            printf(CLR_CURSOR_MARKED " ");
            print_name_clipped(name, s);
            printf(CLR_RESET);
        } else if (real == s->fs.index) {
            printf(CLR_CURSOR " ");
            print_name_clipped(name, s);
            printf(CLR_RESET);
        } else if (s->fs.marked[real]) {
            printf(CLR_MARKED " ");
            print_name_clipped(name, s);
            printf(CLR_RESET);
        } else {
            printf(" ");
            print_name_clipped(name, s);
        }
        printf("\033[%d;%dH", i + 1, s->ui.width_list);
    }

    if (check_dir(s->fs.f_list[s->fs.index]) == 1) {
        draw_file_preview(s->fs.f_list[s->fs.index], s->ui.rows, s->ui.cols);
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
    get_term_size(&st.ui.rows, &st.ui.cols);
    st.fs.marked = calloc(1024, sizeof(int));
    if (!st.fs.marked) {
        perror("calloc");
        return 1;
    }
    st.fs.cwd = malloc(PATH_MAX);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("\033[?1049h");
    printf("\033[?25l");
    fflush(stdout);
    char *f_list[100];
    getcwd(st.fs.cwd, PATH_MAX);
    int items_count = list(st.fs.cwd, f_list);
    st.fs.f_list = f_list;
    st.fs.len = items_count;
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
