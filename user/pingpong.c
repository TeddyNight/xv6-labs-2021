#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p1[2];
    int p2[2];
    int pid;
    pipe(p1);
    pipe(p2);
    if (fork() == 0) {
        pid = getpid();
        char c;
        read(p1[1], &c, 1);
        fprintf(1, "%d: received ping\n", pid);
        write(p2[0], "\n", 1);
    } else {
        pid = getpid();
        write(p1[0], "\n", 1);
        wait(0);
        pid = getpid();
        char c;
        read(p2[1], &c, 1);
        fprintf(1, "%d: received pong\n", pid);
    }
    exit(0);
}
