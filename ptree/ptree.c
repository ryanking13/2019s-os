#include <linux/kernel.h>
#include <linux/prinfo.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/sched/signal.h>
#include <linux/cred.h>
#include <linux/uidgid.h>

int ptree(struct prinfo* buf, int* nr) {

    struct task_struct* task;
    // struct list_head* p;
    int idx = 0;
    int comm_idx = 0;

    // if pointer is null or nr is not positive
    if (buf == NULL || nr == NULL || *nr < 1) {
        return -EINVAL;
    }

    // if address of pointer is invalid
    if (!access_ok(VERIFY_WRITE, nr, sizeof(int)) || !access_ok(VERIFY_WRITE, buf, sizeof(struct prinfo) * (*nr))) {
        return -EFAULT;
    }

    // TODO(mk_rd): change traversing order

    read_lock(&tasklist_lock);
    for_each_process(task) {
        if (idx < *nr) {
            buf->state = (int64_t)task->state;
            buf->pid = task->pid;
            buf->parent_pid = task->real_parent->pid;

            buf->first_child_pid = list_last_entry(&task->children, struct task_struct, children)->pid;
            buf->next_sibling_pid = list_first_entry(&task->sibling, struct task_struct, sibling)->pid;

            // TODO(ddoyoon): check UID is valid
            buf->uid = (int64_t)__kuid_val(task_uid(task));

            while(*(task->comm + comm_idx) != '\0') {
                *(buf->comm + comm_idx) = *(task->comm + comm_idx);
                ++comm_idx;
            }
            *(buf->comm + comm_idx) = '\0';
            comm_idx = 0;
        }

        ++idx;
        ++buf;
    }
    read_unlock(&tasklist_lock);

    return idx;
}