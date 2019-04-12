/* os proj2 rotation lock header file */
#ifndef _LINUX_ROTATION_H
#define _LINUX_ROTATION_H

#include <linux/list.h>
#include <linux/export.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/mutex.h>

/* ===== rotation_state struct related ===== */

void exit_rot_lock(struct task_struct * cur); // process must call this function before die

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
extern rotation_state init_rotation;

inline void rotation_lock(rotation_state* rot);

inline void rotation_unlock(rotation_state* rot);

inline void rotation_set_degree(rotation_state* rot, int degree);

inline int is_device_in_lock_range(int degree, int range, rotation_state* rot);

inline int is_device_in_lock_range_of_lock_entry(rotation_lock_list* entry, rotation_state* rot);

inline int is_lock_ranges_overlap(rotation_lock_list* p, rotation_lock_list* q);

/* ===== rotation_state struct related ===== */

#endif
