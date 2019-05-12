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

    cpu_set_t mask;
    int binded_cpu;

    CPU_ZERO(&mask);
    // if commandline argument is not given, bind this process to CPU 1;
    if (argc > 1) {
        for(int i = 1; i < argc; i++) {
            binded_cpu = atoi(argv[i]);
            CPU_SET(binded_cpu, &mask);
        }
    }
    else {
        CPU_SET(1, &mask);
    }

    // bind this process to specific cpu
    // ret = sched_setaffinity(0, sizeof(mask), &mask);
    // printf("[sched_setaffinity] %d\n", ret);
    // if (ret < 0) {
    //     return 0;
    // }

    const struct sched_param params = {0};

    // check WRR scheduler is properly set
    ret = sched_getscheduler(pid);
    printf("[(before) getscheduler] %d\n", ret);

    // set this process to use WRR scheduler
    ret = sched_setscheduler(pid, SCHED_WRR, &params);
    printf("[setscheduler] %d\n", ret);

    if (ret < 0) {
        return 0;
    }

    // check WRR scheduler is properly set
    ret = sched_getscheduler(pid);
    printf("[(after) getscheduler] %d\n", ret);

    if (ret < 0) {
        return 0;
    }

    // set weight to this process
    ret = SCHED_SETWEIGHT(pid, 15);
    printf("[setweight] %d\n", ret);

    if (ret < 0) {
        return 0;
    }

    pid_t newpid;
    newpid = fork();

    if (newpid == 0) {
        pid = getpid();
        int weight = SCHED_GETWEIGHT(pid);
        printf("Child pid: %d, weight: %d, scheduler: %d\n", pid, weight, sched_getscheduler(pid));
    }

    newpid = fork();
    
    if (newpid == 0) {
        pid = getpid();
        int weight = SCHED_GETWEIGHT(pid);
        printf("Child pid: %d, weight: %d, scheduler: %d\n", pid, weight, sched_getscheduler(pid));
    }

    int cpu;
    int weight;
    for(int i = 0;; i++) {
        if (i % 3000000 == 0) {
            cpu = sched_getcpu();
            weight = SCHED_GETWEIGHT(pid);
            printf("pid: %d cpu: %d ms: %d w: %d\n", pid, cpu, (int)get_time(), weight);
        }
    }
} 