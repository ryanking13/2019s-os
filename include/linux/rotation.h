/* os proj2 rotation lock header file */
#ifndef _LINUX_ROTATION_H
#define _LINUX_ROTATION_H

#include <linux/semaphore.h>
#include <linux/list.h>
#include <linux/export.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/mutex.h>

/* ===== rotation_state struct related ===== */

#define INIT_ROTATION_LOCK_LIST(_name, _degree, _range, _flag) {\
    .degree = _degree,\
    .range = _range,\
    .flag = _flag,\
    .lock_list = LIST_HEAD_INIT(_name.lock_list)\
}

#define INIT_ROTATION_STATE(_name) {\
    .degree = -1,\
    .read_lock_wait_list = INIT_ROTATION_LOCK_LIST(_name.read_lock_wait_list, -1, -1, 0),\
    .write_lock_wait_list = INIT_ROTATION_LOCK_LIST(_name.write_lock_wait_list, -1, -1, 0),\
    .read_lock_list = INIT_ROTATION_LOCK_LIST(_name.read_lock_list, -1, -1, 0),\
    .write_lock_list = INIT_ROTATION_LOCK_LIST(_name.write_lock_list, -1, -1, 0),\
    .lock = __MUTEX_INITIALIZER(_name.lock)\
}

typedef struct {
    int degree;
    int range;
    int flag;  // flag for wait_event
    wait_queue_head_t queue; // wait_event_head that holds this process
    struct list_head lock_list;
    struct task_struct *task_struct;
} rotation_lock_list;

typedef struct {
    int degree;
    rotation_lock_list read_lock_wait_list;
    rotation_lock_list write_lock_wait_list;
    rotation_lock_list read_lock_list;
    rotation_lock_list write_lock_list;
    // spinlock_t lock; // TODO: change it to mutex??
    struct mutex lock;
} rotation_state;

/* global rotation_state struct */
rotation_state init_rotation = INIT_ROTATION_STATE(init_rotation);
EXPORT_SYMBOL(init_rotation);

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

/* ===== rotation_state struct related ===== */

#endif