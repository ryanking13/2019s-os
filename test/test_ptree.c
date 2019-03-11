#define _GNU_SOURCE
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#define SYS_ptree 398

int main() {
    printf("[ptree] ptree syscall test\n");
    int ret = syscall(SYS_ptree, NULL, NULL);
    printf("[ptree] syscall ended with return code %d\n", ret);

    pid_t pid;
    pid = syscall(SYS_getpid);
    printf("[getpid] syscall ended with return code %d\n", pid);
    return 0;
}