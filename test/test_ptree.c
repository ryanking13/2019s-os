#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/prinfo.h>

#define SYS_ptree 398

int main(int argc, char* argv[]) {
    struct prinfo* p;
    int nr;
    int i;

    if (argc == 1) {
        printf("Usage: ./test_ptree <nr>\n");
        return 1;
    } else {
        nr = atoi(argv[1]);
    }
    
    // TODO(ddoyoon): Add testcase (error handling)

    p = (struct prinfo *)malloc(nr * sizeof(struct prinfo));
    printf("[ptree] ptree syscall test\n");
    int ret = syscall(SYS_ptree, p, &nr);
    printf("[ptree] syscall ended with return code %d\n", ret);

    // TODO(ddoyoon): print indented result
    
    for(i = 0; i < nr; ++i) {
        printf("----- prinfo struct [%d] -----\n", i);
        printf("- Command: %s\n", p[i].comm);
        printf("- State: %lld\n", p[i].state);
        printf("- PID: %d\n", p[i].pid);
        printf("  - Parent: %d\n", p[i].parent_pid);
        printf("  - Oldest child: %d\n", p[i].first_child_pid);
        printf("  - Younger sibling: %d\n", p[i].next_sibling_pid);
        printf("- UID: %lld\n", p[i].uid);
        printf("------------------------------\n");
    }

    // pid_t pid;
    // pid = syscall(SYS_getpid);
    // printf("[getpid] syscall ended with return code %d\n", pid);
    return 0;
}
