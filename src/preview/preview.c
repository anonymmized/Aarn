#include "../../headers/preview.h"
#include "../../headers/UI.h"
#include "../../headers/FS.h"
#include "../../headers/rm.h"
#include "../../headers/utils.h"

#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

SortMode g_sort_mode = SORT_NONE;

static volatile sig_atomic_t g_resize_requested = 0;
static int reload_directory(struct AppState *s);
static void rebuild_and_sort_view(struct AppState *s);

static void handle_preview_resize(int signo) {
    (void)signo;
    g_resize_requested = 1;
}

static void free_file_list(struct AppState *s) {
    for (int i = 0; i < s->fs.len; i++) {
        free(s->fs.f_list[i].path);
        s->fs.f_list[i].path = NULL;
    }
    s->fs.len = 0;
    s->fs.view_len = 0;
    s->fs.marked_len = 0;
}

static void recount_marked(struct AppState *s) {
    s->fs.marked_len = 0;
    for (int i = 0; i < s->fs.len; i++) {
        if (s->fs.f_list[i].marked) s->fs.marked_len++;
    }
}

void sync_runtime_mode(struct AppState *s) {
    s->rt.mode = s->fs.marked_len > 0 ? 2 : 0;
}

static void clear_all_marks(struct AppState *s) {
    for (int i = 0; i < s->fs.len; i++) {
        s->fs.f_list[i].marked = 0;
    }
    s->fs.marked_len = 0;
    sync_runtime_mode(s);
}

static void mark_all_items(struct AppState *s) {
    for (int i = 0; i < s->fs.len; i++) {
        s->fs.f_list[i].marked = 1;
    }
    recount_marked(s);
    sync_runtime_mode(s);
}

static void invert_marks(struct AppState *s) {
    for (int i = 0; i < s->fs.len; i++) {
        s->fs.f_list[i].marked ^= 1;
    }
    recount_marked(s);
    sync_runtime_mode(s);
}

static PreviewKey read_escape_key(void) {
    int pending = 0;
    if (ioctl(STDIN_FILENO, FIONREAD, &pending) == -1 || pending < 2) {
        return PK_ESC;
    }

    char seq[2];
    if (read(STDIN_FILENO, &seq[0], 1) <= 0) return PK_ESC;
    if (read(STDIN_FILENO, &seq[1], 1) <= 0) return PK_ESC;
    if (seq[0] != '[') return PK_ESC;

    switch (seq[1]) {
        case 'A':
            return PK_UP;
        case 'B':
            return PK_DOWN;
        case 'C':
            return PK_RIGHT;
        case 'D':
            return PK_LEFT;
        default:
            return PK_ESC;
    }
}

static int confirm_preview_action(struct AppState *s, const char *prompt) {
    printf("\033[%d;2H", s->ui.help_row);
    for (int i = 0; i < s->ui.cols - 3; i++) putchar(' ');
    printf("\033[%d;3H%s [y/n]", s->ui.help_row, prompt);
    fflush(stdout);

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) continue;
        if (c == 'y' || c == 'Y') return 1;
        if (c == 'n' || c == 'N' || c == 'q' || c == '\x1b') return 0;
    }
}

static int delete_marked_items(struct AppState *s) {
    if (s->fs.marked_len == 0) return 0;

    char prompt[128];
    snprintf(prompt, sizeof(prompt), "delete %d marked item%s", s->fs.marked_len, s->fs.marked_len == 1 ? "" : "s");
    int confirmed = confirm_preview_action(s, prompt);
    if (!confirmed) return 0;

    int failed = 0;
    for (int i = 0; i < s->fs.len; i++) {
        if (!s->fs.f_list[i].marked) continue;
        if (remove_item(s->fs.f_list[i].path, 0, 1, 0, 0) != 0) {
            failed = 1;
        }
    }

    reload_directory(s);
    s->rt.mode = 0;
    if (!failed) {
        s->rt.last_key = 'X';
    }
    return failed;
}

static void clamp_selection(struct AppState *s) {
    if (s->fs.view_len <= 0) {
        s->fs.index = 0;
        s->fs.offset = 0;
        return;
    }
    if (s->fs.index < 0) s->fs.index = 0;
    if (s->fs.index >= s->fs.view_len) s->fs.index = s->fs.view_len - 1;
    if (s->fs.offset < 0) s->fs.offset = 0;
    if (s->fs.index < s->fs.offset) s->fs.offset = s->fs.index;
    if (s->fs.index >= s->fs.offset + s->ui.rows) {
        s->fs.offset = s->fs.index - s->ui.rows + 1;
    }
    if (s->fs.offset < 0) s->fs.offset = 0;
}

void refresh_file_scroll(struct AppState *s) {
    if (view_empty(s)) {
        s->fs.file_scroll = 0;
        s->fs.file_lines = 0;
        return;
    }

    if (s->fs.view[s->fs.index]->type == FT_TEXT) {
        s->fs.file_lines = count_file_lines(s->fs.view[s->fs.index]->path);
        if (s->fs.file_scroll > s->fs.file_lines) s->fs.file_scroll = 0;
    } else {
        s->fs.file_scroll = 0;
        s->fs.file_lines = 0;
    }
}

void update_terminal_size(struct AppState *s) {
    get_term_size(&s->ui.rows, &s->ui.cols);
    if (s->ui.rows < 8) s->ui.rows = 8;
    if (s->ui.cols < 40) s->ui.cols = 40;
    s->ui.top_row = 2;
    s->ui.rows -= 5;
    if (s->ui.rows < 1) s->ui.rows = 1;
    s->ui.divider_row = s->ui.top_row + s->ui.rows;
    s->ui.footer_row = s->ui.divider_row + 1;
    s->ui.help_row = s->ui.divider_row + 2;
    s->ui.frame_bottom = s->ui.divider_row + 3;
    s->ui.width_list = (s->ui.cols - 5) / 3;
    if (s->ui.width_list < 10) s->ui.width_list = 10;
    if (s->ui.width_list >= s->ui.cols - 7) s->ui.width_list = s->ui.cols - 7;
    s->ui.cols_preview = s->ui.width_list + 4;
    if (s->ui.cols_preview > s->ui.cols - 3) s->ui.cols_preview = s->ui.cols - 3;
    clamp_selection(s);
}

void swap(void *a, void *b, size_t size) {
    char tmp[size];
    memcpy(tmp, a, size);
    memcpy(a, b, size);
    memcpy(b, tmp, size);
}

int fuzzy_score(const char *s, const char *q) {
    int score = 0;
    int si = 0;
    int qj = 0;
    int last_match = -1;

    while (s[si] && q[qj]) {
        if (tolower((unsigned char)s[si]) == tolower((unsigned char)q[qj])) {
            score += 10;
            if (last_match == si - 1) score += 15;
            if (si == 0 || s[si - 1] == '/' || s[si - 1] == '_' || s[si - 1] == '-') score += 20;
            last_match = si;
            qj++;
        }
        si++;
    }
    if (q[qj] != '\0') return -1;
    return score;
}

int fuzzy_cmp(const void *a, const void *b) {
    FileEntry *path1 = *(FileEntry **)a;
    FileEntry *path2 = *(FileEntry **)b;
    if (path2->score != path1->score) return path2->score - path1->score;
    return strcmp(path1->path, path2->path);
}

int file_cmp_ptr(const void *a, const void *b) {
    const FileEntry *path1 = *(FileEntry **)a;
    const FileEntry *path2 = *(FileEntry **)b;

    const char *name1 = strrchr(path1->path, '/');
    const char *name2 = strrchr(path2->path, '/');

    name1 = name1 ? name1 + 1 : path1->path;
    name2 = name2 ? name2 + 1 : path2->path;

    if (g_sort_mode == SORT_NONE) return strcmp(name1, name2);
    if (path1->type == FT_DIR && path2->type != FT_DIR) return -1;
    if (path1->type != FT_DIR && path2->type == FT_DIR) return 1;

    switch (g_sort_mode) {
        case SORT_NAME_ASC:
            return strcmp(name1, name2);
        case SORT_NAME_DESC:
            return strcmp(name2, name1);
        case SORT_DATE_DESC:
        case SORT_DATE_ASC: {
            struct stat st1;
            struct stat st2;
            if (stat(path1->path, &st1) != 0) return 0;
            if (stat(path2->path, &st2) != 0) return 0;
            if (g_sort_mode == SORT_DATE_DESC) {
                return (st2.st_mtime > st1.st_mtime) - (st2.st_mtime < st1.st_mtime);
            }
            return (st1.st_mtime > st2.st_mtime) - (st1.st_mtime < st2.st_mtime);
        }
        default:
            return strcmp(name1, name2);
    }
}

void rebuild_view(struct AppState *s) {
    FileEntry **tmp = realloc(s->fs.view, sizeof(FileEntry *) * (s->fs.len > 0 ? s->fs.len : 1));
    if (!tmp && s->fs.len > 0) {
        return;
    }
    s->fs.view = tmp;
    for (int i = 0; i < s->fs.len; i++) s->fs.view[i] = &s->fs.f_list[i];
    s->fs.view_len = s->fs.len;
    recount_marked(s);
    clamp_selection(s);
}

void quick_sort(void *base, int left, int right, size_t size, int (*cmp)(const void *, const void *)) {
    char *arr = (char *)base;
    int i = left;
    int j = right;
    char pivot[size];

    memcpy(pivot, arr + ((left + right) / 2) * size, size);
    while (i <= j) {
        while (cmp(arr + i * size, pivot) < 0) i++;
        while (cmp(arr + j * size, pivot) > 0) j--;

        if (i <= j) {
            swap(arr + i * size, arr + j * size, size);
            i++;
            j--;
        }
    }
    if (left < j) quick_sort(base, left, j, size, cmp);
    if (right > i) quick_sort(base, i, right, size, cmp);
}

void filter_view(struct AppState *s, char *pattern) {
    rebuild_view(s);
    s->fs.view_len = 0;
    if (!pattern || pattern[0] == '\0') {
        for (int i = 0; i < s->fs.len; i++) s->fs.view[s->fs.view_len++] = &s->fs.f_list[i];
        return;
    }
    for (int i = 0; i < s->fs.len; i++) {
        FileEntry *entry = &s->fs.f_list[i];
        char *name = strrchr(entry->path, '/');
        name = name ? name + 1 : entry->path;
        int score = fuzzy_score(name, pattern);
        if (score >= 0) {
            entry->score = score;
            s->fs.view[s->fs.view_len++] = entry;
        }
    }
}

void sort_view(struct AppState *s, int (*cmp)(const void *, const void *)) {
    if (!s->fs.view || s->fs.view_len <= 1) return;
    quick_sort(s->fs.view, 0, s->fs.view_len - 1, sizeof(FileEntry *), cmp);
}

static void rebuild_and_sort_view(struct AppState *s) {
    if (s->fs.enter_search && s->fs.enter_search[0] != '\0') {
        filter_view(s, s->fs.enter_search);
        sort_view(s, fuzzy_cmp);
    } else {
        rebuild_view(s);
        sort_view(s, file_cmp_ptr);
    }
    recount_marked(s);
    clamp_selection(s);
    refresh_file_scroll(s);
    sync_runtime_mode(s);
}

static int reload_directory(struct AppState *s) {
    free_file_list(s);
    if (list(s) < 0) return 1;
    rebuild_and_sort_view(s);
    s->fs.index = 0;
    s->fs.offset = 0;
    refresh_file_scroll(s);
    return 0;
}

static void handle_resize_if_needed(struct AppState *s) {
    if (!g_resize_requested) return;
    g_resize_requested = 0;
    update_terminal_size(s);
    redraw(s);
}

static void move_selection(struct AppState *s, int delta) {
    if (view_empty(s)) return;

    int next = s->fs.index + delta;
    if (next < 0) next = s->fs.view_len - 1;
    if (next >= s->fs.view_len) next = 0;

    if (next != s->fs.index) {
        s->fs.index = next;
        s->fs.file_scroll = 0;
        refresh_file_scroll(s);
    }
    clamp_selection(s);
}

static void toggle_mark(struct AppState *s) {
    if (view_empty(s)) return;

    FileEntry *entry = s->fs.view[s->fs.index];
    entry->marked ^= 1;
    if (entry->marked) s->fs.marked_len++;
    else if (s->fs.marked_len > 0) s->fs.marked_len--;

    sync_runtime_mode(s);
}

static int enter_directory(struct AppState *s, const char *path) {
    if (chdir(path) == -1) return 1;
    if (!getcwd(s->fs.cwd, PATH_MAX)) return 1;
    s->rt.mode = 0;
    s->fs.file_scroll = 0;
    s->fs.file_lines = 0;
    return reload_directory(s);
}

static void leave_search_mode(struct AppState *s, int clear_query) {
    if (clear_query && s->fs.enter_search) {
        s->fs.enter_search[0] = '\0';
    }
    if (clear_query) {
        s->fs.last_entered = 0;
    }
    rebuild_and_sort_view(s);
    redraw(s);
}

void input_monitor(struct AppState *s) {
    struct sigaction sa = {0};
    struct sigaction old_winch = {0};

    sa.sa_handler = handle_preview_resize;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGWINCH, &sa, &old_winch);

    s->fs.index = 0;
    s->fs.offset = 0;
    s->rt.launched = 1;
    s->rt.mode = 0;
    free(s->fs.enter_search);
    s->fs.enter_search = NULL;
    s->fs.last_entered = 0;
    s->fs.file_scroll = 0;
    s->fs.file_lines = 0;
    update_terminal_size(s);
    rebuild_and_sort_view(s);
    redraw(s);

    while (1) {
        handle_resize_if_needed(s);

        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            handle_resize_if_needed(s);
            continue;
        }

        s->rt.last_key = c;

        if (s->rt.mode == 4) {
            if (c == 'q') {
                sync_runtime_mode(s);
                redraw(s);
                continue;
            }
            if (c == '\x1b') {
                PreviewKey key = read_escape_key();
                int max_scroll = s->fs.file_lines - s->ui.rows;
                if (max_scroll < 0) max_scroll = 0;

                if (key == PK_UP) {
                    s->fs.file_scroll = s->fs.file_scroll > 0 ? s->fs.file_scroll - 1 : max_scroll;
                    redraw(s);
                } else if (key == PK_DOWN) {
                    s->fs.file_scroll = s->fs.file_scroll < max_scroll ? s->fs.file_scroll + 1 : 0;
                    redraw(s);
                } else if (key == PK_ESC) {
                    sync_runtime_mode(s);
                    redraw(s);
                }
            }
            continue;
        }

        if (s->rt.mode == 3) {
            if (c == 27) {
                leave_search_mode(s, 1);
                continue;
            }
            if (c == '\n' || c == '\r') {
                leave_search_mode(s, 0);
                continue;
            }

            if (c == 127 || c == 8) {
                if (s->fs.last_entered > 0) {
                    s->fs.last_entered--;
                    s->fs.enter_search[s->fs.last_entered] = '\0';
                }
            } else if (c >= 32 && c <= 126 && s->fs.last_entered < 1023) {
                s->fs.enter_search[s->fs.last_entered++] = c;
                s->fs.enter_search[s->fs.last_entered] = '\0';
            }

            rebuild_and_sort_view(s);
            redraw(s);
            continue;
        }

        int is_sort = 1;
        if (c == 'a') g_sort_mode = SORT_NAME_ASC;
        else if (c == 'A') g_sort_mode = SORT_NAME_DESC;
        else if (c == 'd') g_sort_mode = SORT_DATE_ASC;
        else if (c == 'D') g_sort_mode = SORT_DATE_DESC;
        else is_sort = 0;

        if (is_sort) {
            s->fs.index = 0;
            s->fs.offset = 0;
            rebuild_and_sort_view(s);
            redraw(s);
            continue;
        }

        if (c == 's') {
            rebuild_and_sort_view(s);
            redraw(s);
            continue;
        }
        if (c == ':') {
            s->rt.mode = 3;
            free(s->fs.enter_search);
            s->fs.enter_search = calloc(1024, 1);
            if (!s->fs.enter_search) {
                s->rt.mode = 0;
                redraw(s);
                continue;
            }
            s->fs.last_entered = 0;
            redraw(s);
            continue;
        }
        if (c == '\n' || c == '\r') {
            if (view_empty(s)) continue;
            s->rt.last_key = 'E';
            const char *path = s->fs.view[s->fs.index]->path;
            if (s->fs.view[s->fs.index]->type == FT_DIR) {
                if (enter_directory(s, path) == 0) redraw(s);
            } else if (s->fs.view[s->fs.index]->type == FT_TEXT) {
                s->rt.mode = 4;
                s->fs.file_scroll = 0;
                refresh_file_scroll(s);
                redraw(s);
            } else {
                redraw(s);
            }
            continue;
        }
        if (c == 127 || c == 8) {
            s->rt.last_key = 'B';
            if (strcmp(s->fs.cwd, "/") != 0 && chdir("..") == 0) {
                getcwd(s->fs.cwd, PATH_MAX);
                reload_directory(s);
                redraw(s);
            }
            continue;
        }
        if (c == 'm') {
            mark_all_items(s);
            redraw(s);
            continue;
        }
        if (c == 'u') {
            clear_all_marks(s);
            redraw(s);
            continue;
        }
        if (c == 'i') {
            invert_marks(s);
            redraw(s);
            continue;
        }
        if (c == 'x' && s->fs.marked_len > 0) {
            delete_marked_items(s);
            redraw(s);
            continue;
        }
        if (c == 'q') {
            s->rt.last_key = 'Q';
            s->rt.launched = 0;
            break;
        }
        if (c == ' ') {
            s->rt.last_key = 'S';
            toggle_mark(s);
            redraw(s);
            continue;
        }
        if (c == '\x1b') {
            PreviewKey key = read_escape_key();
            if (key == PK_ESC) {
                if (s->fs.marked_len > 0) {
                    clear_all_marks(s);
                    redraw(s);
                }
                continue;
            }

            if (key == PK_UP) {
                s->rt.last_key = 'U';
                move_selection(s, -1);
                redraw(s);
            } else if (key == PK_DOWN) {
                s->rt.last_key = 'D';
                move_selection(s, 1);
                redraw(s);
            } else if (key == PK_RIGHT) {
                s->rt.last_key = 'R';
                if (!view_empty(s) && s->fs.view[s->fs.index]->type == FT_DIR) {
                    if (enter_directory(s, s->fs.view[s->fs.index]->path) == 0) redraw(s);
                } else {
                    redraw(s);
                }
            } else if (key == PK_LEFT) {
                s->rt.last_key = 'L';
                if (strcmp(s->fs.cwd, "/") != 0 && chdir("..") == 0) {
                    getcwd(s->fs.cwd, PATH_MAX);
                    reload_directory(s);
                    redraw(s);
                }
            }
        }
    }

    sigaction(SIGWINCH, &old_winch, NULL);
    s->rt.launched = 0;
}

void redraw(struct AppState *s) {
    clamp_selection(s);
    s->ui.width_list = (s->ui.cols - 5) / 3;
    if (s->ui.width_list < 10) s->ui.width_list = 10;
    if (s->ui.width_list >= s->ui.cols - 7) s->ui.width_list = s->ui.cols - 7;
    s->ui.cols_preview = s->ui.width_list + 4;
    if (s->ui.cols_preview > s->ui.cols - 3) s->ui.cols_preview = s->ui.cols - 3;

    for (int r = 1; r <= s->ui.frame_bottom; r++) {
        printf("\033[%d;1H\033[K", r);
    }
    draw_frame(s);
    clear_preview_area(s);

    if (view_empty(s)) {
        draw_statusbar(s);
        fflush(stdout);
        return;
    }

    for (int i = 0; i < s->ui.rows; i++) {
        s->fs.real = s->fs.offset + i;
        if (s->fs.real >= s->fs.view_len) break;

        printf("\033[%d;2H", s->ui.top_row + i);

        if (s->fs.real == s->fs.index && s->fs.view[s->fs.real]->marked) {
            printf(CLR_CURSOR_MARKED " ");
            print_name_clipped(s);
            printf(CLR_RESET);
        } else if (s->fs.real == s->fs.index) {
            printf(CLR_CURSOR " ");
            print_name_clipped(s);
            printf(CLR_RESET);
        } else if (s->fs.view[s->fs.real]->marked) {
            printf(CLR_MARKED " ");
            print_name_clipped(s);
            printf(CLR_RESET);
        } else {
            printf(" ");
            print_name_clipped(s);
        }
        printf("\033[%d;%dH", s->ui.top_row + i, s->ui.cols_preview - 2);
    }

    s->ui.preview_st = 0;
    FileEntry *cur = s->fs.view[s->fs.index];
    if (cur->type == FT_BINARY) {
        printf("\033[%d;%dH<BINARY FILE>", s->ui.top_row, s->ui.cols_preview + 1);
    } else if (cur->type == FT_TEXT) {
        draw_file_preview(s, s->fs.file_scroll);
        s->ui.preview_st = 1;
    }

    draw_statusbar(s);
    fflush(stdout);
}
