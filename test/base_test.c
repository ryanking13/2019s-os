#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sched.h>
#include <uapi/linux/sched.h>
#include <sys/time.h>

#define SCHED_SETWEIGHT(pid, weight) syscall(398, pid, weight)
#define SCHED_GETWEIGHT(pid) syscall(399, pid)

double get_time() {
    struct timeval t;
    double dtime;
    gettimeofday(&t, NULL);
    dtime = t.tv_sec * 1000 + (t.tv_usec / 1000);
    return dtime;
}


int main(int argc, char **argv) {
    int ret = -1;

    pid_t pid = getpid();
    const struct sched_param params = {0};

    // set this process to use WRR scheduler
    ret = sched_setscheduler(pid, SCHED_WRR, &params);
    printf("[sched_setscheduler] %d\n", ret);

    if (ret < 0) {
        return 0;
    }

    // check WRR scheduler is properly set
    ret = sched_getscheduler(pid);
    printf("[sched_getscheduler] %d\n", ret);

    if (ret < 0) {
        return 0;
    }

    cpu_set_t mask;
    int binded_cpu;

    // if commandline argument is not given, bind this process to CPU 1;
    if (argc > 1) binded_cpu = atoi(argv[1]);
    else binded_cpu = 1;

    CPU_ZERO(&mask);
    CPU_SET(binded_cpu, &mask);

    // bind this process to specific cpu
    ret = sched_setaffinity(0, sizeof(mask), &mask);
    printf("[sched_setaffinity] %d\n", ret);
    if (ret < 0) {
        return 0;
    }

    // set weight to this process
    ret = SCHED_SETWEIGHT(pid, 15);
    printf("[setweight] %d\n", ret);

    if (ret < 0) {
        return 0;
    }

    int cpu;
    while(1) {
        cpu = sched_getcpu();
        printf("%d %f\n", cpu, get_time());
    }
} 