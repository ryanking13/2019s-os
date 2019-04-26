#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SCHED_SETWEIGHT(pid, weight) syscall(398, pid, weight)
#define SCHED_GETWEIGHT(pid) syscall(399, pid)

int main() {
    int ret = -1;

    ret = SCHED_SETWEIGHT(1, 1);
    printf("[sched_setweight] ended with return code: %d\n", ret);
    ret = SCHED_GETWEIGHT(2);
	printf("[sched_getweight] ended with return code: %d\n", ret);
} 