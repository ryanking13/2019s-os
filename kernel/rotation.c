#include <linux/syscalls.h>

/*
 * sets the current device rotation in the kernel.
 * syscall number 398 (you may want to check this number!)
 */
long set_rotation(int degree) { /* 0 <= degree < 360 */
    return 0;
}

/*
 * Take a read/or write lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 399 and 400
 */
long rotlock_read(int degree, int range) {  /* 0 <= degree < 360 , 0 < range < 180 */
    return 0;
}
long rotlock_write(int degree, int range) { /* degree - range <= LOCK RANGE <= degree + range */
    return 0;
}

/*
 * Release a read/or write lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 401 and 402
 */
long rotunlock_read(int degree, int range) {  /* 0 <= degree < 360 , 0 < range < 180 */
    return 0;
}
long rotunlock_write(int degree, int range) { /* degree - range <= LOCK RANGE <= degree + range */
    return 0;
}

SYSCALL_DEFINE1(set_rotation, int, degree)
{
    return set_rotation(degree);
}

SYSCALL_DEFINE2(rotlock_read, int, degree, int, range)
{
    return rotlock_read(degree, range);
}

SYSCALL_DEFINE2(rotlock_write, int, degree, int, range)
{
    return rotlock_write(degree, range);
}

SYSCALL_DEFINE2(rotunlock_read, int, degree, int, range)
{
    return rotunlock_read(degree, range);
}

SYSCALL_DEFINE2(rotunlock_write, int, degree, int, range)
{
    return rotunlock_write(degree, range);
}