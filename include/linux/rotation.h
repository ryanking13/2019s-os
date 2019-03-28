/* os proj2 rotation lock header file */
#ifndef _LINUX_ROTATION_H
#define _LINUX_ROTATION_H

#include <linux/semaphore.h>
#include <linux/list.h>
#include <linux/export.h>

/* ===== rotation_state struct related ===== */
typedef struct {
    int degree;
    int range;
    // TODO: need an attribute to identify which thread this is?
    struct list_head next;
} rotation_lock_list;

typedef struct {
    int degree;
    rotation_lock_list read_lock_wait_list;
    rotation_lock_list write_lock_wait_list;
    rotation_lock_list read_lock_list;
    rotation_lock_list write_lock_list;
    spinlock_t lock; // TODO: change it to read/write lock??
} rotation_state;

/* global rotation_state struct */
rotation_state init_rotation;
EXPORT_SYMBOL(init_rotation);

inline void rotation_lock(rotation_state* rot) {
    spin_lock(&rot->lock);
}

inline void rotation_unlock(rotation_state* rot) {
    spin_unlock(&rot->lock);
}

inline void rotation_set_degree(rotation_state* rot, int degree) {
    rot->degree = degree;
}

inline int is_device_in_rotation(rotation_lock_list* node, rotation_state* rot) {
    // TODO: check if rotation is inside nodes' LOCK RANGE
    return 1;
}

/* ===== rotation_state struct related ===== */

#endif