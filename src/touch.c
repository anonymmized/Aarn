#include "../headers/touch.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int atoi_n(const char *s, int pos, int len) {
    char tmp[5];
    memcpy(tmp, s + pos, len);
    tmp[len] = '\0';
    return atoi(tmp);
}

int touch_file(const char *path, char *stamp, int atime, int mtime, int has_stamp) {
    time_t t;
    if (has_stamp) {
        int time_len = strlen(stamp);
        char *dot = strchr(stamp, '.');
        int has_sec = (dot != NULL);
        int pos = 0;
        int year = 0;
        if (time_len < 10) {
            fprintf(stderr, "touch: invalid time format\n");
            return 1;
        }
        if (time_len >= 12) {
            year = atoi_n(stamp, 0, 4);
            pos = 4;
        } else {
            int yy = atoi_n(stamp, 0, 2);
            year = (yy >= 69) ? 1900 + yy : 2000 + yy;
            pos = 2;
        }
        int mon = atoi_n(stamp, pos, 2);
        int day = atoi_n(stamp, pos + 2, 2);
        int hour = atoi_n(stamp, pos + 4, 2);
        int min = atoi_n(stamp, pos + 6, 2);
        int sec = has_sec ? atoi(dot + 1) : 0;
        struct tm tm = {0};
        tm.tm_year = year - 1900;
        tm.tm_mon = mon - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;
        tm.tm_isdst = -1;
        t = mktime(&tm);
        if (t == (time_t)-1) {
            fprintf(stderr, "touch: invalid timestamp\n");
            return 1;
        }
    } else {
        t = time(NULL);
    }

    if (!atime && !mtime) {
        atime = 1;
        mtime = 1;
    }

    if (access(path, F_OK) != 0) {
        int fd = open(path, O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
            perror("open");
            return 1;
        }
        close(fd);
    } 

    struct timespec times[2];
    
    if (atime) {
        times[0].tv_sec = t;
        times[0].tv_nsec = 0;
    } else {
        times[0].tv_nsec = UTIME_OMIT;
    }

    if (mtime) {
        times[1].tv_sec = t;
        times[1].tv_nsec = 0;
    } else {
        times[1].tv_nsec = UTIME_OMIT;
    }

    if (utimensat(AT_FDCWD, path, times, 0) == -1) {
        perror("utimensat");
        return 1;
    }

    return 0;
}

int parse_touch_flags(int argc, char **argv, int *start) {
    int flags = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][0] != '\0') {
        for (int j = 1; argv[i][j] != '\0'; j++) {
            char c = argv[i][j];
            if (c == 'a') flags |= TOUCH_ATIME;
            else if (c == 'm') flags |= TOUCH_MTIME;
            else if (c == 't') flags |= TOUCH_STAMP;
            else {
                fprintf(stderr, "touch: unknown option -%c\n", c);
                return -1;
            }
        }
        i++;
    }
    *start = i;
    return flags;
}


