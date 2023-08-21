#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int time;
    if (argc <= 1 || argc >= 3) {
        fprintf(2, "sleep: require an argument\n");
        exit(1);
    }
    time = atoi(argv[1]); 
    if (time < 0) {
        fprintf(2, "sleep: invalid argument\n");
        exit(1);
    }
    sleep(time);
    exit(0);
}
