#include "../../headers/FS.h"
#include "../../headers/preview.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

int list(struct AppState *s) {
    DIR *wdir = opendir(s->fs.cwd);
    if (!wdir) {
        fprintf(stderr, "Error wit opening dir\n");
        return 0;
    }
    s->fs.len = 0;
    struct dirent *ent;
    while ((ent = readdir(wdir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        if (s->fs.len >= s->fs.capacity) {
            s->fs.capacity *= 2;
            char **tmp = realloc(s->fs.f_list, sizeof(char *) * s->fs.capacity);
            if (!tmp) {
                perror("malloc");
                closedir(wdir);
                return s->fs.len;
            }
            s->fs.f_list = tmp;
        }
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", s->fs.cwd, ent->d_name);
        s->fs.f_list[s->fs.len] = malloc(strlen(fullpath) + 1);
        strcpy(s->fs.f_list[s->fs.len++], fullpath);

    }
    closedir(wdir);
    return s->fs.len;
}

int fs_empty(struct AppState *s) {
    return s->fs.len == 0;
}

void update_current_file_type(struct AppState *s) {
    FileType t = get_file_type(s->fs.f_list[s->fs.index]);
    s->fs.type = t;
}

FileType get_file_type(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return FT_DIR;
        }
        else if (!S_ISREG(st.st_mode)) {
            return FT_UNKNOWN;
        }
        else  {
            FILE *fp = fopen(path, "rb");
            if (!fp) {
                return FT_UNKNOWN;
            }
            unsigned char buf[512];
            size_t n = fread(buf, 1, sizeof(buf), fp);
            fclose(fp);
            for (size_t i = 0; i < n; i++) {
                if (buf[i] == 0) {
                    return FT_BINARY;
                }
            }
            const char *dot = strrchr(path, '.');
            if (!dot || dot == path) {
                return FT_TEXT;
            }
            else dot++;
            const char *ext = dot;
            if (!strcmp(ext, "pdf")) {
                return FT_BINARY;
            }
            if (!strcmp(ext, "png")) {
                return FT_BINARY;
            }
            if (!strcmp(ext, "jpg")) {
                return FT_BINARY;
            }
            if (!strcmp(ext, "jpeg")) {
                return FT_BINARY;
            }
            if (!strcmp(ext, "zip")) {
                return FT_BINARY;
            }
            if (!strcmp(ext, "tar")) {
                return FT_BINARY;
            }
            if (!strcmp(ext, "gz")) {
                return FT_BINARY;
            }
            if (!strcmp(ext, "mp4")) {
                return FT_BINARY;
            }
            if (!strcmp(ext, "mp3")) {
                return FT_BINARY;
            }
            return FT_TEXT;
        }
    }
    return FT_UNKNOWN;
}
