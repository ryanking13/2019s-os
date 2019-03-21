#ifdef __KERNEL__
#include<linux/types.h>
#else
#include<sys/types.h>
#endif

#ifndef __PRINFO__
#define __PRINFO__

struct prinfo {
    int64_t state;
    pid_t pid;
    pid_t parent_pid;
    pid_t first_child_pid;
    pid_t next_sibling_pid;
    int64_t uid;
    char comm[64];
};

#endif
