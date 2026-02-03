#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

int atoi_n(const char *s, int pos, int len) {
    char tmp[5];
    memcpy(tmp, s + pos, len);
    tmp[len] = '\0';
    return atoi(tmp);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("bad input\n");
        return 1;
    }
    if (strcmp(argv[1], "-t") != 0) {
        printf("bad input\n");
        return 1;
    }
    int time_len = strlen(argv[2]);
    char *dot = strchr(argv[2], '.');
    int has_sec = (dot != NULL);
    int pos = 0;
    int year = 0;
    if (time_len < 10) {
        printf("bad input time\n");
        return 1;
    }
    if (time_len >= 12) {
        year = atoi_n(argv[2], 0, 4);
        pos = 4;
    } else if (time_len == 10) {
        int yy = atoi_n(argv[2], 0, 2);
        year = (yy >= 69) ? 1900 + yy : 2000 + yy;
        pos = 2;
    }
    int mon = atoi_n(argv[2], pos, 2);
    int day = atoi_n(argv[2], pos + 2, 2);
    int hour = atoi_n(argv[2], pos + 4, 2);
    int min = atoi_n(argv[2], pos + 6, 2);
    int sec = has_sec ? atoi(dot + 1) : 0;
    struct tm tm = {0};
    tm.tm_year = year - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = -1;
    printf("YY:%d MM:%d DD:%d hh:%d mm:%d ss:%d\n", tm.tm_year - 100, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    time_t t = mktime(&tm);
    printf("Seconds: %ld", t);
}
