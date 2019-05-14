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

    // TEST1 : higher / lower weight with WRR process(curr process)
    printf("===============================================\n");
    printf("TEST1 - increase & decrease weight with curr process\n");

    if(ret = SCHED_SETWEIGHT(pid, 5)<0)
        printf("weight decreasing failed\n");

    for(int i=0;i<100000000;i++)
        ; //spin

    for(int t=0;!(ret<0)&&t<4; t++)
    {
        for(int i = 0; i < 500000000; i++) {
            if (i % 100000000 == 0) {
                cpu = sched_getcpu();
                weight = SCHED_GETWEIGHT(pid);
                printf("pid: %d\tcpu: %d\tweight: %d\n", pid, cpu, weight);
            }
        }
        if(t%2){
            if(ret = SCHED_SETWEIGHT(pid,15)<0){
                printf("weight increasing failed\n");
                break;
            }
        }
        else{
            if(ret = SCHED_SETWEIGHT(pid,5)<0){
                printf("weight decreasing failed\n");
                break;
            }
        }
    }

    // TEST2 : get / set weight fail test
    printf("===============================================\n");
    printf("TEST2 - get / set weight fail test\n");

    printf("if process is not working on WRR\n");
    printf("suppose pid 1 process (kthreadd) is not working on WRR\n");
    if(weight = SCHED_GETWEIGHT(1)<0)
        printf("getweight failed\n");
    if(ret = SCHED_SETWEIGHT(1,15)<0)
        printf("setweight failed\n");

    printf("if process does not exist\n");
    if(weight = SCHED_GETWEIGHT(2147483647)<0)
        printf("getweight failed\n");
    if(ret = SCHED_SETWEIGHT(2147483647,15)<0)
        printf("setweight failed\n");

    printf("if given weight is not in specific range [1,20]");
    if(ret = SCHED_SETWEIGHT(pid, 30)<0)
        printf("setweight failed\n");

} 
