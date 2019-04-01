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

void factorize(int num) {
    int first = 1;
    int div = 2;
    while(num > 1) {
        while (num % div == 0) {
            num /= div;
            if (first) {
                printf("%d", div);
                first = 0;
            } else {
                printf(" * %d", div);
            }
        }
        div++;
    }
}

int main(int argc, char *argv[]) {

    /* TODO: remove after handled auto unlock */
    signal(SIGINT, (void *)sig_handler);
    signal(SIGTERM, (void *)sig_handler);

    if (argc == 1) {
        printf("[-] Usage: %s <starting number>\n", argv[0]);
        return 1;
    }

    int id = atoi(argv[1]);
    if (argv[1][0] != '0' && id == 0) {
        printf("[-] wrong commandline argument: %s\n", argv[1]);
    }

    char pname[100];
    sprintf(pname, "%s-%d", argv[0], id);

    FILE *fp;
    int ret;
    for(; !done /* TODO: remove after handled auto unlock */;) {
        ret = ROTLOCK_READ(90, 90);
        // printf("[*] rotlock_read ended: %d\n", ret);
        fp = fopen("integer", "rt");

        if (fp == NULL) {
            printf("[-] fopen Error\n");
            return 1;
        }

        int num;
        fscanf(fp, "%d", &num);
        printf("%s: %d = ", pname, num);
        factorize(num);
        printf("\n");

        fclose(fp);
        ROTUNLOCK_READ(90, 90);
        // printf("[*] rotunlock_read ended: %d\n", ret);
    }

    return 0;
}