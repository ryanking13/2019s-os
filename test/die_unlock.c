#define _GNU_SOURCE
#include <sys/syscall.h>
#include <unistd.h>

#define ROTLOCK_WRITE(degree, range) syscall(400, degree, range)

int main(int argc, char * argv[])
{
    ROTLOCK_WRITE(180,10);
    return 0;
}
