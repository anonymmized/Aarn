#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#include "../headers/utils.h"
#include "../headers/pwd.h"
#include "../headers/cd.h"
#include "../headers/cat.h"
#include "../headers/list.h"
#include "../headers/rm.h"
#include "../headers/mkdir.h"

#define MAXLINE 1024

#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define ITALIC "\033[3m"
#define ESC    "\033[0m"

int main() {
    const char *workin = return_last_dir(print_workin());
    printf(BOLD "%s > " ESC, workin);
    const char *current_dir;
    if ((current_dir = print_workin()) == NULL) {
        fprintf(stderr, "Error with opening dir\n");
        return 1;
    }
    char *c_d = (char *)current_dir;
    char line[MAXLINE];
    int len;
    while ((len = get_line(line, MAXLINE)) > 0) {
        char *argv[16];
        int argc = parse_line(line, argv, 16);
        int output_code = get_command_id(argv[0]);
        switch (output_code) {
            case CMD_exit:
                fprintf(stdout, "exit in progress...\n");
                return 0;
            case CMD_pwd: { 
                const char *pwd_v;
                if ((pwd_v = print_workin()) != NULL) {
                    fprintf(stdout, "%s\n", pwd_v);
                    break;
                } else {
                    perror("pwd");
                    break;
                }
            }
            case CMD_clear:
                system("clear");
                break;
            case CMD_ls: { 
                int start = 1;
                int flags = parse_ls_flags(argc, argv, &start);
                if (flags == -1) break;

                int hidden = (flags & LS_A) != 0;
                if (start >= argc) {
                    list_wd(".", hidden);
                } else {
                    for (int i = start; i < argc; i++) {
                        if (argc - start > 1) {
                            printf("%s:\n", argv[i]);
                        }
                        list_wd(argv[i], hidden);
                        if (i + 1 < argc) putchar('\n');
                    }
                }
                break;
            }
            case CMD_cd:
                if (argc >= 2) {
                    char *arg = argv[1];
                    cmd_cd(arg);
                    workin = return_last_dir(print_workin());
                    break;
                } else {
                    cmd_cd(" ");
                    break;
                }
            case CMD_cat:
                if (argc == 2) {
                    char *filename = argv[1];
                    if (cat_file(filename) == 1) {
                        fprintf(stderr, "Error reading file: %s\n", argv[1]);
                        break;
                    }
                } else {
                    for (int i = 1; i < argc; i++) {
                        if (cat_file(argv[i]) != 0) {
                            fprintf(stderr, "Error reading file: %s\n", argv[i]);
                            break;
                        }
                    } 
                }
                break;
            case CMD_rm: {
                int start = 1;

                int flags = parse_rm_flags(argc, argv, &start);
                if (flags == -1) break;

                int confirm = (flags & RM_I) != 0;
                int recursive = (flags & RM_R) != 0;
                int force = (flags & RM_F) != 0;
                int interactive = (flags & RM_INTER) != 0;
                int verbose = (flags & RM_V) != 0;
                
                if (force) {
                    confirm = 0;
                    interactive = 0;
                }

                if (start >= argc) {
                    fprintf(stderr, "rm: missing operand\n");
                    break;
                }
                int targets = argc - start;
                if (interactive && (recursive || targets > 3)) {
                    printf("remove: ");
                    for (int i = start; i < argc; i++) {
                        printf("%s ", argv[i]);
                    }
                    printf("[y|n] ");
                    char ans[10];
                    fgets(ans, sizeof(ans), stdin);
                    if (ans[0] != 'y' && ans[0] != 'Y') {
                        break;
                    }
                }

                for (int i = start; i < argc; i++) {
                    remove_item(argv[i], confirm, recursive, force, verbose);
                }
                break;
            }
            case CMD_mkdir:
                for (int i = 1; i < argc; i++) {
                    create_dir(argv[i]);
                }
                break;
            default:
                printf("command not found: %s\n", argv[0]);
                break;
        }
        printf(BOLD "%s > " ESC, workin);
    }
    return 0;
}
