#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SET_ROTATION(degree) syscall(398, degree)
#define ROTLOCK_READ(degree, range) syscall(399, degree, range)
#define ROTLOCK_WRITE(degree, range) syscall(400, degree, range)
#define ROTUNLOCK_READ(degree, range) syscall(401, degree, range)
#define ROTUNLOCK_WRITE(degree, range) syscall(402, degree, range)

int main() {
    int ret = -1;

    ret = SET_ROTATION(90);
    printf("[set_rotation] ended with return code: %d\n", ret);
    ret = ROTLOCK_READ(90, 90);
    printf("[rotlock_read] ended with return code: %d\n", ret);
    ret = ROTUNLOCK_READ(90, 90);
    printf("[rotunlock_read] ended with return code: %d\n", ret);
    ret = ROTLOCK_WRITE(90, 90);
    printf("[rotlock_write] ended with return code: %d\n", ret);
    ret = ROTUNLOCK_WRITE(90, 90);
    printf("[rotunlock_write] ended with return code: %d\n", ret);
}