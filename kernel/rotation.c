#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/rotation.h>
#include <linux/list.h>
#include <linux/wait.h>

// TODO: handle process die auto unlocking

rotation_state init_rotation = INIT_ROTATION_STATE(init_rotation);
EXPORT_SYMBOL(init_rotation);

/* iterate through waiting lock list and wake up if in valid range */
int wake_up_wait_list(rotation_state *rot) {
    rotation_lock_list* lock_entry;
    rotation_lock_list* _lock_entry; // temporary storage for list_for_each_entry_safe
    int reader_holding = 0;
    int unlocked = 0;

    // if there is a writer holding a lock in current degree,
    // no release so nothing happens
    list_for_each_entry(lock_entry, &rot->write_lock_list.lock_list, lock_list) {
        if (is_device_in_lock_range_of_lock_entry(lock_entry, rot)) {
            return 0;
        }
    }

    // if there is a writer holding a lock in current degree,
    // other readers can hold that lock.
    list_for_each_entry(lock_entry, &rot->read_lock_list.lock_list, lock_list) {
        if (is_device_in_lock_range_of_lock_entry(lock_entry, rot)) {
            reader_holding = 1;
        }
    }

    // if reader is not holding a lock, then iterate through waiting writers
    // and release very first wating lock where lock range overlaps current degree
    if(!reader_holding) {
        list_for_each_entry_safe(lock_entry, _lock_entry, &rot->write_lock_wait_list.lock_list, lock_list) {
            if (is_device_in_lock_range_of_lock_entry(lock_entry, rot)) {
                unlocked += 1;
                lock_entry->flag = 1;
                wake_up_interruptible(&lock_entry->queue);
                list_del(&lock_entry->lock_list);
                list_add_tail(&lock_entry->lock_list, &rot->write_lock_list.lock_list);
                return unlocked;
            }
        }
    }

    list_for_each_entry_safe(lock_entry, _lock_entry, &rot->read_lock_wait_list.lock_list, lock_list) {
        if (is_device_in_lock_range_of_lock_entry(lock_entry, rot)) {
            unlocked += 1;
            lock_entry->flag = 1;
            wake_up_interruptible(&lock_entry->queue);
            list_del(&lock_entry->lock_list);
            list_add_tail(&lock_entry->lock_list, &rot->read_lock_list.lock_list);
        }
    }

    return unlocked;

    /*
        0. check if there some writer is hold a lock for current degree, if so, nothing happens
        1. check if there are waiting readers
        2. check if there are waiting writers
        3. select reader's' or writer to give lock
        4. awake selected readers/writer
    */
}

/* release locks on die */
void exit_rot_lock(struct task_struct * cur) {
    // TODO: this function is not tested
    rotation_state *rot = &init_rotation;
    rotation_lock_list* lock_entry;
    rotation_lock_list* _lock_entry; // temporary storage for list_for_each_entry_safe

    rotation_lock(rot);

    list_for_each_entry_safe(lock_entry, _lock_entry, &rot->write_lock_list.lock_list, lock_list) {
        if (lock_entry->task_struct->tgid == cur->tgid) {
            list_del(&lock_entry->lock_list);
            kfree(lock_entry);
        }
    }

    list_for_each_entry_safe(lock_entry, _lock_entry, &rot->read_lock_list.lock_list, lock_list) {
        if (lock_entry->task_struct->tgid == cur->tgid) {
            list_del(&lock_entry->lock_list);
            kfree(lock_entry);
        }
    }

    list_for_each_entry_safe(lock_entry, _lock_entry, &rot->write_lock_wait_list.lock_list, lock_list) {
        if (lock_entry->task_struct->tgid == cur->tgid) {
            list_del(&lock_entry->lock_list);
            kfree(lock_entry);
        }
    }

    list_for_each_entry_safe(lock_entry, _lock_entry, &rot->read_lock_wait_list.lock_list, lock_list) {
        if (lock_entry->task_struct->tgid == cur->tgid) {
            list_del(&lock_entry->lock_list);
            kfree(lock_entry);
        }
    }

    wake_up_wait_list(rot);
    rotation_unlock(rot);
    return;
}

/*
 * sets the current device rotation in the kernel.
 * syscall number 398 (you may want to check this number!)
 */
long set_rotation(int degree) { /* 0 <= degree < 360 */
    rotation_state *rot = &init_rotation;
    int unlocked = 0;

    rotation_lock(rot);
    rotation_set_degree(rot, degree);
    unlocked = wake_up_wait_list(rot);
    rotation_unlock(rot);

    return unlocked;
}

/*
 * Take a read/or write lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 399 and 400
 */
long rotlock_read(int degree, int range) {  /* 0 <= degree < 360 , 0 < range < 180 */
    rotation_state *rot = &init_rotation;
    rotation_lock_list* lock_entry;
    rotation_lock_list* new_lock_entry;

    // invalid degree / range
    if (!(0 <= degree && degree < 360) || !(0 < range && range < 180)) {
        return -1;
    }

    // memory for new entry must be allocated here outside of lock
    new_lock_entry = (rotation_lock_list *)kmalloc(sizeof(rotation_lock_list), GFP_KERNEL);
    new_lock_entry->degree = degree;
    new_lock_entry->range = range;
    new_lock_entry->flag = 0;
    new_lock_entry->task_struct = current;
    init_waitqueue_head(&new_lock_entry->queue);
    INIT_LIST_HEAD(&new_lock_entry->lock_list);

    rotation_lock(rot);

    // device is not in lock range, queue this process to wait list
    if (!is_device_in_lock_range(degree, range, rot)) {
        list_add_tail(&new_lock_entry->lock_list, &rot->read_lock_wait_list.lock_list);
        rotation_unlock(rot);
        wait_event_interruptible(new_lock_entry->queue, (new_lock_entry->flag == 1));
        return 0;
    }

    // if there is a waiting write lock where lock range overlapps current device rotation
    // queue this process to wait list
    list_for_each_entry(lock_entry, &rot->write_lock_wait_list.lock_list, lock_list) {
        if (is_device_in_lock_range_of_lock_entry(lock_entry, rot)) {
            list_add_tail(&new_lock_entry->lock_list, &rot->read_lock_wait_list.lock_list);
            rotation_unlock(rot);
            wait_event_interruptible(new_lock_entry->queue, (new_lock_entry->flag == 1));
            return 0;
        }
    }

    // if there is a write lock holding lock where lock range overlapps new lock
    // queue this process to wait list 
    list_for_each_entry(lock_entry, &rot->write_lock_list.lock_list, lock_list) {
        if (is_lock_ranges_overlap(lock_entry, new_lock_entry)) {
            list_add_tail(&new_lock_entry->lock_list, &rot->read_lock_wait_list.lock_list);
            rotation_unlock(rot);
            wait_event_interruptible(new_lock_entry->queue, (new_lock_entry->flag == 1));
            return 0;            
        }
    }

    list_add_tail(&new_lock_entry->lock_list, &rot->read_lock_list.lock_list);
    rotation_unlock(rot);
    return 0;

    // if not in LOCK_RANGE
    //     queue this thread
    // for writer_wait_list
    //     if there is waiting lock in current rotation, queue this thread
    // for writer_list
    //     if there is overlapping lock, queue this thread
    // else
    //     take a reader lock and run this thread

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
            3.1. wrong degree / range
    */
}
long rotlock_write(int degree, int range) { /* degree - range <= LOCK RANGE <= degree + range */
    rotation_state *rot = &init_rotation;
    rotation_lock_list* lock_entry;
    rotation_lock_list* new_lock_entry;

    // invalid degree / range
    if (!(0 <= degree && degree < 360) || !(0 < range && range < 180)) {
        return -1;
    }

    // memory for new entry must be allocated here outside of lock
    new_lock_entry = (rotation_lock_list *)kmalloc(sizeof(rotation_lock_list), GFP_KERNEL);
    new_lock_entry->degree = degree;
    new_lock_entry->range = range;
    new_lock_entry->flag = 0;
    new_lock_entry->task_struct = current;
    init_waitqueue_head(&new_lock_entry->queue);
    INIT_LIST_HEAD(&new_lock_entry->lock_list);

    rotation_lock(rot);

    // device is not in lock range, queue this process to wait list
    if (!is_device_in_lock_range(degree, range, rot)) {
        // printk(KERN_INFO "[rotlock_write] rotation not in lock range: %d\n", rot->degree);
        list_add_tail(&new_lock_entry->lock_list, &rot->write_lock_wait_list.lock_list);
        rotation_unlock(rot);
        wait_event_interruptible(new_lock_entry->queue, (new_lock_entry->flag == 1));
        return 0;
    }

    // if there is a waiting write lock where lock range overlapps current device rotation
    // queue this process to wait list
    list_for_each_entry(lock_entry, &rot->write_lock_wait_list.lock_list, lock_list) {
        if (is_device_in_lock_range_of_lock_entry(lock_entry, rot)) {
            // printk(KERN_INFO "[rotlock_write] there is other waiting write lock: %d\n", rot->degree);
            list_add_tail(&new_lock_entry->lock_list, &rot->write_lock_wait_list.lock_list);
            rotation_unlock(rot);
            wait_event_interruptible(new_lock_entry->queue, (new_lock_entry->flag == 1));
            return 0;
        }
    }

    // if there is a write lock holding lock where lock range overlapps new lock
    // queue this process to wait list 
    list_for_each_entry(lock_entry, &rot->write_lock_list.lock_list, lock_list) {
        if (is_lock_ranges_overlap(lock_entry, new_lock_entry)) {
            // printk(KERN_INFO "[rotlock_write] waiting other write lock: %d\n", rot->degree);
            list_add_tail(&new_lock_entry->lock_list, &rot->write_lock_wait_list.lock_list);
            rotation_unlock(rot);
            wait_event_interruptible(new_lock_entry->queue, (new_lock_entry->flag == 1));
            return 0;            
        }
    }

    // if there is a write lock holding lock where lock range overlapps new lock
    // queue this process to wait list 
    list_for_each_entry(lock_entry, &rot->read_lock_list.lock_list, lock_list) {
        if (is_lock_ranges_overlap(lock_entry, new_lock_entry)) {
            // printk(KERN_INFO "[rotlock_write] waiting other read lock: %d\n", rot->degree);
            list_add_tail(&new_lock_entry->lock_list, &rot->write_lock_wait_list.lock_list);
            rotation_unlock(rot);
            wait_event_interruptible(new_lock_entry->queue, (new_lock_entry->flag == 1));
            return 0;            
        }
    }

    list_add_tail(&new_lock_entry->lock_list, &rot->write_lock_list.lock_list);
    rotation_unlock(rot);
    return 0;


    // if not in LOCK_RANGE
    //     queue this thread
    // for writer_list
    //     if there is overlapping lock, queue this thread
    // for reader_list
    //     if there is overlapping lock, queue this thread
    // else
    //     take a writer lock and run this thread

    /*
        1. NO_BLOCK CASE (device must be in LOCK_RANGE)
            1.1. no other thread is holding a lock.
            1.2. other threads are holding a lock.
                1.2.1. current lock is not overlapping
        2. BLOCK_CASE
            2.1. add this thread to wait_list and go sleep.
        3. FAIL_CASE
            3.1. wrong degree / range
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
    rotation_lock_list* lock_entry;
    rotation_lock_list* _lock_entry; // temporary storage for list_for_each_entry_safe
    struct task_struct *cur;

    // invalid degree / range
    if (!(0 <= degree && degree < 360) || !(0 < range && range < 180)) {
        return -1;
    }

    cur = current;
    rotation_lock(rot);

    list_for_each_entry_safe(lock_entry, _lock_entry, &rot->read_lock_list.lock_list, lock_list) {
        if (lock_entry->task_struct->tgid == cur->tgid &&\
            lock_entry->degree == degree && lock_entry->range == range) {
            list_del(&lock_entry->lock_list);
            kfree(lock_entry);
            wake_up_wait_list(rot);
            rotation_unlock(rot);
            return 0;
        }
    }

    // there was no lock to unlock (invalid case), return -1
    rotation_unlock(rot);
    return -1;

    // remove this thread from reader list
    // then, same as what set_rotation do
}
long rotunlock_write(int degree, int range) { /* degree - range <= LOCK RANGE <= degree + range */
    rotation_state *rot = &init_rotation;
    rotation_lock_list* lock_entry;
    rotation_lock_list* _lock_entry; // temporary storage for list_for_each_entry_safe
    struct task_struct *cur;

    // invalid degree / range
    if (!(0 <= degree && degree < 360) || !(0 < range && range < 180)) {
        return -1;
    }

    cur = current;
    rotation_lock(rot);

    list_for_each_entry_safe(lock_entry, _lock_entry, &rot->write_lock_list.lock_list, lock_list) {
        if (lock_entry->task_struct->tgid == cur->tgid &&\
            lock_entry->degree == degree && lock_entry->range == range) {
            list_del(&lock_entry->lock_list);
            kfree(lock_entry);
            wake_up_wait_list(rot);
            rotation_unlock(rot);
            return 0;
        }
    }

    // there was no lock to unlock (invalid case), return -1
    rotation_unlock(rot);
    return -1;

    // remove this thread from reader list
    // then, same as what set_rotation do
}

inline void rotation_lock(rotation_state* rot) {
    // spin_lock(&rot->lock);
    mutex_lock(&rot->lock);
}

inline void rotation_unlock(rotation_state* rot) {
    // spin_unlock(&rot->lock);
    mutex_unlock(&rot->lock);
}

inline void rotation_set_degree(rotation_state* rot, int degree) {
    rot->degree = degree;
}

inline int is_device_in_lock_range(int degree, int range, rotation_state* rot) {
    int diff_cw = abs(rot->degree - degree);
    int diff_ccw = 360 - diff_cw;

    if (diff_cw <= range || diff_ccw <= range)
        return 1;

    return 0;
}

inline int is_device_in_lock_range_of_lock_entry(rotation_lock_list* entry, rotation_state* rot) {
    return is_device_in_lock_range(entry->degree, entry->range, rot);
}

inline int is_lock_ranges_overlap(rotation_lock_list* p, rotation_lock_list* q) {
    int diff_cw = abs(p->degree -  q->degree);
    int diff_ccw = 360 - diff_cw;
    int range = p->range + q->range;

    if (diff_cw <= range || diff_ccw <= range)
        return 1;

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
