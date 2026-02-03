#include "../headers/touch.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int touch_file(const char *path, int atime, int mtime) {
    int existed = (access(path, F_OK) == 0);
    if (!existed) {
        int fd = open(path, O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
            perror("open");
            return 1;
        }
        close(fd);
    }

    struct timespec times[2];
    times[0].tv_nsec = UTIME_NOW;
    times[1].tv_nsec = UTIME_NOW;
    if (atime == 1 && mtime == 0) {
        times[0].tv_sec = time(NULL);
        times[0].tv_nsec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }
    if (mtime == 1 && atime == 0) {
        times[0].tv_nsec = UTIME_OMIT;
        times[1].tv_sec = time(NULL);
        times[1].tv_nsec = 0;
    }

    if (utimensat(AT_FDCWD, path, times, 0) == -1) {
        perror("utimensat");
        return 1;
    }

    return 0;
}
