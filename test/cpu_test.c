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

    // check WRR scheduler is properly set
    ret = sched_getscheduler(pid);
    if (ret < 0) return 0;

    // TEST1 : CPU0 masking (unavailable cpu) test
    printf("===============================================\n");
    printf("TEST1 - CPU0 masking (unavailable cpu) test\n");

    cpu_set_t mask1;
    CPU_ZERO(&mask1);
    CPU_SET(0, &mask1);

    const struct sched_param params1 = {0};

    // set this process to use RR scheduler
    ret = sched_setscheduler(pid, SCHED_RR, &params1);
    if (ret < 0) return 0;
    
    for(int t=0;t<100000000;t++)
        ; //spin

    ret = sched_setscheduler(pid, SCHED_WRR, &params1);
    if (ret < 0)
        printf("Change RR scheduler to WRR failed with CPU0\n");

    cpu_set_t mask2;
    CPU_ZERO(&mask2);
    CPU_SET(1, &mask2);
    CPU_SET(2, &mask2);
    CPU_SET(3, &mask2);
    
    const struct sched_param params2 = {0};
    int flag = 0;

    ret = sched_setscheduler(pid, SCHED_WRR, &params2);

    for(int t=0;t<10;t++)
    {
        for(int i=0;i<500000000;i++)
        {
            if (i%100000000 ==0)
            {
                cpu = sched_getcpu();
                if(cpu == 0)
                {
                    flag = 1;
                    break;
                }
            }
        }
        if(flag)
            break;
    }
    if(flag)
        printf("CPU0 is assigned. test failed\n");
    else
        printf("CPU0 is not assigned. test passed\n");

} 
