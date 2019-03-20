#include <linux/kernel.h>
#include <linux/prinfo.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/sched/signal.h>
#include <linux/cred.h>
#include <linux/uidgid.h>
#include <linux/slab.h>

int ptree(struct prinfo* buf, int* nr) {

    struct task_struct* task;
    struct task_struct* next_task;
    int process_cnt = 0;
    int comm_idx = 0;
    struct prinfo* _buf; // temp buf
    struct prinfo* p;
    int _nr; // temp nr
    int err;
    int write_en = 1;

    // if pointer is null, return error
    if (buf == NULL || nr == NULL) return -EINVAL;
    // if address of nr is invalid, return error
    if (!access_ok(VERIFY_WRITE, nr, sizeof(int))) return -EFAULT;

    get_user(_nr, nr);

    // nr is not positive, return error
    if (_nr < 1) return -EINVAL;
    // if address of buf is invalid, return error
    if (!access_ok(VERIFY_WRITE, buf, sizeof(struct prinfo) * (_nr))) return -EFAULT;

    _buf = (struct prinfo *)kmalloc(sizeof(struct prinfo) * (_nr), GFP_KERNEL);
    if (!_buf) return -ENOMEM;

    // TODO(mk_rd): change traversing order

    p = _buf;

    read_lock(&tasklist_lock);
    task = init_task;
    do {
        next_task = NULL;
        
        if (list_empty(&task->sibling) || list_is_last(task->sibling,task->parent->children)) {
            if (write_en)
                p->next_sibling_pid = 0;
        } else {
            next_task = list_next_entry(task, sibling);
            if (write_en)
                p->next_sibling_pid = next_task->pid;
        }
        
        if(list_empty(&task->children)) {
            if (write_en)
                p->first_child_pid = 0;
        } else {
            next_task = list_first_entry(&task->children, struct task_struct, sibling);
            if (write_en)
                p->first_child_pid = next_task->pid;
        }

        if (write_en) {
            p->uid = (int64_t)__kuid_val(task_uid(task));
            p->state = (int64_t)task->state;
            p->pid = task->pid;
            p->parent_pid = task->parent->pid;

            while(*(task->comm + comm_idx) != '\0') {
                *(p->comm + comm_idx) = *(task->comm + comm_idx);
                ++comm_idx;
            }
            *(p->comm + comm_idx) = '\0';
            comm_idx = 0;
            ++p;
        }

        ++process_cnt;

        if(next_task == NULL) {
            next_task = task;
            while(next_task->pid && list_is_last(next_task->sibling, next_task->parent->children))
                next_task = next_task->parent;
            if(next_task->pid)
                next_task = list_next_entry(next_task, sibling);
        }

        if(write_en && process_cnt >= _nr)
            write_en = 0;
    } while(((task = next_task)->pid))

    read_unlock(&tasklist_lock);

    // copy temp values to user space
    err = copy_to_user(buf, _buf, sizeof(struct prinfo) * (_nr));
    if (err < 0) return err;
    kfree(_buf);

    // if nr value is bigger than whole process
    if (process_cnt < _nr) _nr = process_cnt;
    put_user(_nr, nr);

    return process_cnt;
}
