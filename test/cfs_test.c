#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sched.h>
#include <uapi/linux/sched.h>
#include <linux/getcpu.h>

// normal CFS task
int main() {

    int cpu;
    int print_interval = 10000000;
    for(int i = 0;; i++) {
        if (i % print_interval == 0) {
            cpu = sched_getcpu();
            if (cpu < 0) {
                printf("FAIL TO CHECK CPU\n");
            }
            else {
                printf("count: %d / cpu: %d\n", i / print_interval, cpu);
            }
        }
    }
}