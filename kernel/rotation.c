#include <linux/syscalls.h>
#include <linux/rotation.h>

// TODO: user memory -> kernel memory copy

/*
 * sets the current device rotation in the kernel.
 * syscall number 398 (you may want to check this number!)
 */
long set_rotation(int degree) { /* 0 <= degree < 360 */
    rotation_state *rot = &init_rotation;

    rotation_lock(rot);
    rotation_set_degree(rot, degree);
    /* TODO
        0. check if there some reader/writer is hold a lock for current degree, if so, nothing happens
        1. check if there are waiting readers
        2. check if there are waiting writers
        3. select reader's' or writer to give lock
        4. awake selected readers/writer
    */
    rotation_unlock(rot);

    return 0;
}

/*
 * Take a read/or write lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 399 and 400
 */
long rotlock_read(int degree, int range) {  /* 0 <= degree < 360 , 0 < range < 180 */
    rotation_state *rot = &init_rotation;

    rotation_lock(rot);
    // for writer_wait_list
    //     if there is overlapping lock, fail
    // for writer_list
    //     if there is overlapping lock, queue this thread
    // if not in LOCK_RANGE
    //     queue this thread
    // else
    //     take a reader lock and run this thread
    rotation_unlock(rot);

    /* TODO
        1. NO_BLOCK CASE (device must be in LOCK_RANGE)
            1.1. no other thread is holding a lock.
            1.2. other threads are holding a lock.
                1.2.1. other threads holding a lock are readers
                    1.2.1.1. there is no writer waiting in overlapping range
                1.2.2. other thread holding a lock is writer
                    1.2.2.1. there is no writer waiting in overlapping range & current lock is not overlapping
        2. BLOCK_CASE
            2.1. add this thread to wait_list and go sleep.
        3. FAIL_CASE
            3.1. if writer in overlapping range is waiting, it fails.
    */

    return 0;
}
long rotlock_write(int degree, int range) { /* degree - range <= LOCK RANGE <= degree + range */
    rotation_state *rot = &init_rotation;

    rotation_lock(rot);
    // for writer_list
    //     if there is overlapping lock, queue this thread
    // for reader_list
    //     if there is overlapping lock, queue this thread
    // if not in LOCK_RANGE
    //     queue this thread
    // else
    //     take a writer lock and run this thread
    rotation_unlock(rot);
    /* TODO
        1. NO_BLOCK CASE (device must be in LOCK_RANGE)
            1.1. no other thread is holding a lock.
            1.2. other threads are holding a lock.
                1.2.1. current lock is not overlapping
        2. BLOCK_CASE
            2.1. add this thread to wait_list and go sleep.
        3. FAIL_CASE
            3.1. maybe no fail case?
    */
    return 0;
}

/*
 * Release a read/or write lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 401 and 402
 */
long rotunlock_read(int degree, int range) {  /* 0 <= degree < 360 , 0 < range < 180 */
    rotation_state *rot = &init_rotation;

    rotation_lock(rot);
    // remove this thread from reader list
    // then, same as set_rotation
    rotation_unlock(rot);
    return 0;
}
long rotunlock_write(int degree, int range) { /* degree - range <= LOCK RANGE <= degree + range */
    rotation_state *rot = &init_rotation;

    rotation_lock(rot);
    // remove this thread from reader list
    // then, same as set_rotation
    rotation_unlock(rot);
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