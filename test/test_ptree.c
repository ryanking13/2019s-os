#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/prinfo.h>
#include <errno.h>


#define SYS_ptree 398

int main(int argc, char* argv[]) {
    struct prinfo* p;
    int nr;
    int valid_nr = 100;
    int invalid_nr = 0;
    int i;
    int ret;
    int* nr_p;

    // if (argc == 1) {
    //     printf("Usage: ./test_ptree <nr>\n");
    //     return 1;
    // } else {
    //     nr = atoi(argv[1]);
    // }

    printf("[ptree] ptree syscall error handling test\n\n");

    printf("Case 1: nr < 1 (-EINVAL)\n");
    p = (struct prinfo *)malloc(valid_nr * sizeof(struct prinfo));
    printf("input: buf=%p, &nr=%p\n", p, &invalid_nr);
    ret = syscall(SYS_ptree, p, &invalid_nr);
    printf("[ptree] syscall ended with return code %d\n", ret);
    if (ret < 0) {
        perror("[ptree] error");
    }
    free(p);
    printf("\n");

    printf("Case 2: buf == NULL (-EINVAL)\n");
    p = NULL;
    printf("input: buf=%p, &nr=%p\n", p, &valid_nr);
    ret = syscall(SYS_ptree, p, &valid_nr);
    printf("[ptree] syscall ended with return code %d\n", ret);
    if (ret < 0) {
        perror("[ptree] error");
    }
    printf("\n");

    printf("Case 3: &nr == NULL (-EINVAL)\n");
    p = (struct prinfo *)malloc(valid_nr * sizeof(struct prinfo));
    nr_p = NULL;
    printf("input: buf=%p, &nr=%p\n", p, nr_p);
    ret = syscall(SYS_ptree, p, nr_p);
    printf("[ptree] syscall ended with return code %d\n", ret);
    if (ret < 0) {
        perror("[ptree] error");
    }
    free(p);
    printf("\n");

    printf("Case 4: buf out of range (-EFAULT)\n");
    p = (struct prinfo*)0xfffffff0;
    printf("input: buf=%p, &nr=%p\n", p, &valid_nr);
    ret = syscall(SYS_ptree, p, &valid_nr);
    printf("[ptree] syscall ended with return code %d\n", ret);
    if (ret < 0) {
        perror("[ptree] error");
    }
    printf("\n");

    printf("Case 5: &nr out of range (-EFAULT)\n");
    p = (struct prinfo *)malloc(nr * sizeof(struct prinfo));
    nr_p = (int*)0xfffffff0;
    printf("input: buf=%p, &nr=%p\n", p, nr_p);
    ret = syscall(SYS_ptree, p, nr_p);
    printf("[ptree] syscall ended with return code %d\n", ret);
    if (ret < 0) {
        perror("[ptree] error");
    }
    free(p);
    printf("\n");

    printf("[ptree] ptree syscall error handling test done\n");
    printf("[ptree] ptree syscall test\n");

    while(1){
        printf("nr: ");
        scanf("%d", &nr);
        p = (struct prinfo *)malloc(nr * sizeof(struct prinfo));
        ret = syscall(SYS_ptree, p, &nr);

        if (ret < 0) {
            perror("[ptree] error");
            continue;
        }
        printf("[ptree] syscall ended with return code %d\n", ret);

        pid_t depth[nr];
        int cursor = -1;

        // for(i = 0; i < nr; ++i) {
        //     printf("----- prinfo struct [%d] -----\n", i);
        //     printf("- Command: %s\n", p[i].comm);
        //     printf("- State: %lld\n", p[i].state);
        //     printf("- PID: %d\n", p[i].pid);
        //     printf("  - Parent: %d\n", p[i].parent_pid);
        //     printf("  - Oldest child: %d\n", p[i].first_child_pid);
        //     printf("  - Younger sibling: %d\n", p[i].next_sibling_pid);
        //     printf("- UID: %lld\n", p[i].uid);
        //     printf("------------------------------\n");
        // }

        for(i = 0; i < nr; ++i) {
            if(i == 0) {
                depth[i] = p[i].pid;
            }

            while(depth[cursor] != p[i].parent_pid && cursor > 0) cursor--;

            cursor++;
            depth[cursor] = p[i].pid;

            for(int indent = 0; indent < cursor; indent++) printf("\t");
            printf("%s,%d,%lld,%d,%d,%d,%lld\n", p[i].comm, p[i].pid, p[i].state, p[i].parent_pid,
                p[i].first_child_pid, p[i].next_sibling_pid, p[i].uid);
        }
        printf("\n");
        free(p);
        // pid_t pid;
        // pid = syscall(SYS_getpid);
        // printf("[getpid] syscall ended with return code %d\n", pid);
        // return 0;
    }
}
