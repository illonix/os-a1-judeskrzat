#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc != 2 || argv[1][0] == '\0') {
        fprintf(2, "usage: sleep <ticks>\n");
        exit(1);
    }

    int ticks = 0;
    for (int i = 0; argv[1][i] != '\0'; i++) {
        if (argv[1][i] < '0' || argv[1][i] > '9') {
            fprintf(2, "usage: sleep <ticks>\n");
            exit(1);
        }

        int digit = argv[1][i] - '0';
        if (ticks > (0x7fffffff - digit) / 10) {
            fprintf(2, "usage: sleep <ticks>\n");
            exit(1);
        }
        ticks = ticks * 10 + digit;
    }

    pause(ticks);
    exit(0);
}