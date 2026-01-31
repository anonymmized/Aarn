#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

struct termios orig;

void enable_raw() {
    tcgetattr(STDIN_FILENO, &orig);
    struct termios raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

void redraw(const char *buf) {
    printf("\r\033[KcliFM> %s", buf);
    fflush(stdout);
}

void load_string(char **history, int index, char *buf, int *len) {
    strcpy(buf, history[index]);
    *len = strlen(buf);
    redraw(buf);
}

int main() {
    char buf[1024];
    int len = 0;
    buf[0] = '\0';
    char *history[100];
    int history_index = 0;
    int history_len = 0;
    enable_raw();
    redraw("");
    while (1) {
        char c;
        read(STDIN_FILENO, &c, 1);
        if (c == '\n') {
            buf[len] = '\0';
            history[history_len++] = strdup(buf);
            history_index = history_len;
            if (strcmp(buf, "exit") == 0) {
                printf("\n"); 
                break;
            }
            len = 0;
            printf("\n");
            redraw("");
            continue;
        } else if (c == 127 || c == 8) {
            if (len > 0) {
                len--;
                buf[len] = '\0';
                history_index = history_len;
                redraw(buf);
            }
        } else if (c == '\x1b') {
            char seq[2];
            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);
            if (seq[1] == 'A') {
                if (history_len > 0 && history_index > 0) {
                    history_index--;
                    load_string(history, history_index, buf, &len);
                }
            }
            if (seq[1] == 'B') {
                if (history_index < history_len - 1) {
                    history_index++;
                    load_string(history, history_index, buf, &len);
                } else if (history_index == history_len - 1) {
                    history_index = history_len;
                    len = 0;
                    buf[len] = '\0';
                    redraw("");
                }
            }
        } else if (c >= 32 && c < 127) {
            if (len < (int)sizeof(buf) - 1) {
                history_index = history_len;
                buf[len++] = c;
                buf[len] = '\0';
                redraw(buf);
            }
        }
    }
    disable_raw();
    return 0;                                                               
}
