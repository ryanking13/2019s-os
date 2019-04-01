#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

/* TODO: remove this after handled auto unlock */
#include <signal.h>
int done = 0;
void sig_handler(int signo)
{
  done = 1;
}
/* ========================================== */

#define SET_ROTATION(degree) syscall(398, degree)
#define ROTLOCK_READ(degree, range) syscall(399, degree, range)
#define ROTLOCK_WRITE(degree, range) syscall(400, degree, range)
#define ROTUNLOCK_READ(degree, range) syscall(401, degree, range)
#define ROTUNLOCK_WRITE(degree, range) syscall(402, degree, range)

int main(int argc, char *argv[]) {

    /* TODO: remove after handled auto unlock */
    signal(SIGINT, (void *)sig_handler);
    signal(SIGTERM, (void *)sig_handler);

    if (argc == 1) {
        printf("[-] Usage: ./%s <starting number>\n", argv[0]);
        return 1;
    }

    int num = atoi(argv[1]);
    if (argv[1][0] != '0' && num == 0) {
        printf("[-] wrong commandline argument: %s\n", argv[1]);
    }

    FILE *fp;
    int ret;
    for(; !done /* TODO: remove after handled auto unlock */; ++num) {
        ret = ROTLOCK_WRITE(90, 90);
        // printf("[*] rotlock_write ended: %d\n", ret);
        fp = fopen("integer", "wt");

        if (fp == NULL) {
            printf("[-] fopen Error\n");
            return 1;
        }

        fprintf(fp, "%d\n", num);
        fclose(fp);
        ROTUNLOCK_WRITE(90, 90);
        // printf("[*] rotunlock_write ended: %d\n", ret);
    }

    return 0;
}