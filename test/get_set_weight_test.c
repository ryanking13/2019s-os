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
    printf("TEST1 - higher & lower weight with curr process\n");

    SCHED_SETWEIGHT(pid, 5);

    for(int i=0;i<100000000;i++)
        ; //spin

    for(int t=0;i<4; i++)
    {
        for(int i = 0; i < 500000000; i++) {
            if (i % 100000000 == 0) {
                cpu = sched_getcpu();
                weight = SCHED_GETWEIGHT(pid);
                printf("pid: %d\tcpu: %d\tweight: %d\n", pid, cpu);
            }
        }
        if(t%2)
            SCHED_SETWEIGHT(pid,15);
        else
            SCHED_SETWEIGHT(pid,5);
    }

    // TEST2 : get / set weight fail test
    printf("===============================================\n");
    printf("TEST2 - get / set weight fail test\n");

    printf("if process is not working with WRR scheduler\n");
    weight = SCHED_GETWEIGHT(1);
    weight = SCHED_SETWEIGHT(1,15);

    printf("if process does not exist\n");
    weight = SCHED_GETWEIGHT(2147483647);
    weight = SCHED_SETWEIGHT(2147483647,15);

    printf("if given weight is not in specific range [1,20]");
    weight = SCHED_SETWEIGHT(pid, 30);

} 
