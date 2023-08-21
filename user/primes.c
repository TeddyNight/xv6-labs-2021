#include "kernel/types.h"
#include "user/user.h"
#define N 34

void
primeProc(int *in, int *out, int num)
{
    int n;
    while (read(in[0], &n, 4) != 0) {
        if (n == num) {
            fprintf(1, "prime %d\n", n);
        } else if (n % num != 0) {
            write(out[1], &n, 4);
        }
    }
    exit(0);
}

int
main(int argc, char *argv[])
{
    int p[2][2];
    pipe(p[0]);

    for (int i = 0; i < N; i++) {
        int num = i + 2;
        write(p[0][1], &num, 4);
    }

    for (int i = 0; i < N; i++) {
        close(p[i % 2][1]);
        pipe(p[(i + 1) % 2]);
        if (fork() == 0) {
            primeProc(p[i % 2], p[(i + 1) % 2], i + 2);
        } else {
            wait(0);
            close(p[i % 2][0]);
        }
    }
    exit(0);
}
