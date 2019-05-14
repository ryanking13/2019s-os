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

    if(argc<2)
        return 0;

    pid_t pid = atoi(argv[1]);

    cpu_set_t mask;

    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    CPU_SET(2, &mask);
    CPU_SET(3, &mask);

    const struct sched_param params = {0};

    // TEST1 : SETWEIGHT test of specific pid
    printf("===============================================\n");
    printf("TEST1 - SETWEIGHT test of specific pid\n");

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
                printf("pid: %d\tcpu: %d\tweight: %d\n", pid, cpu);
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
} 
