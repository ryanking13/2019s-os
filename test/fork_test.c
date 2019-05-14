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

int main(int argc, char **argv) {
    int ret = -1;
    int cpu;
    int weight;

    pid_t pid = getpid();
    pid_t child_pid;

    cpu_set_t mask;

    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    CPU_SET(2, &mask);
    CPU_SET(3, &mask);

    const struct sched_param params = {0};

    // set this process to use WRR scheduler
    ret = sched_setscheduler(pid, SCHED_WRR, &params);
    if (ret < 0) return 0;

    // check WRR scheduler is properly set
    ret = sched_getscheduler(pid);
    if (ret < 0) return 0;

    if(ret = SCHED_SETWEIGHT(pid, 7)<0)
        printf("weight decreasing failed\n");
 
    // TEST1 : fork test
    printf("===============================================\n");
    printf("TEST1 - fork test\n");


    child_pid = fork();
    if(child_pid==0)
    {
        weight = SCHED_GETWEIGHT(pid);
        printf("parent weight is %d\n", weight);
    }
    else
    {
        weight = SCHED_GETWEIGHT(child_pid);
        printf("child weight is %d\n", weight);
    }
    
    for(int i=0;i<500000000;i++)
        ; //spin
} 
