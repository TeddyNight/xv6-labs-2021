#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int
readline(int old_argc, char **buf)
{
    int argc = old_argc;
    for (int i = old_argc; i < MAXARG; i++) {
        for (int j = 0; j < MAXARG; j++) {
            if (read(0, (char *)buf + argc * MAXARG + j, 1) == 0) {
                return argc;
            }
            char c = *((char *)buf + argc * MAXARG + j);
            if (c == ' ') {
                *((char *)buf + argc * MAXARG + j) = 0;
                argc++;
                break;
            }
            if (c == '\n') {
                *((char *)buf + argc * MAXARG + j) = 0;
                argc++;
                return argc;
            }
            if (c == '\0') {
                argc++;
                break;
            }
        }
    }
    return argc;
}

int
main(int argc, char *argv[])
{
    char buf[MAXARG][MAXARG];
    int addition_argc = MAXARG - argc + 1;
    if (argc <= 1) {
        fprintf(2, "xargs [-n MAX_ARGS] [command]\n");
        exit(1);
    }
    int old_argc = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && argc >= 3) {
            addition_argc = atoi(argv[i + 1]);
            if (addition_argc <= 0 || addition_argc > MAXARG - argc + 1) {
                fprintf(2, "xargs: invalid argument\n");
                exit(1);
            }
            i++;
        } else {
            memcpy(buf[old_argc], argv[i], strlen(argv[i]) + 1);
            old_argc++;
        }
    }
    int new_argc;
    while ((new_argc = readline(old_argc, (char **)buf)) != old_argc) {
        if (fork() == 0) {
            char *new_argv[new_argc + 1];
            for (int i = 0; i < new_argc; i++) {
                new_argv[i] = (char *)buf + i * MAXARG;
            }
            new_argv[new_argc] = 0;
            // C11 5.1.2.2.1 Program startup
            exec(new_argv[0], new_argv);
            printf("xargs: exec %s error\n", buf[0]);
        } else {
            wait(0);
        }
    }
    exit(0);
}
