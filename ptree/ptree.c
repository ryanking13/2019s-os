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

    task = list_first_entry(&find_task_by_vpid(1)->parent->children, struct task_struct, sibling);
    while(task->pid && process_cnt < _nr) {
        p->state = (int64_t)task->state;
        p->pid = task->pid;
        p->parent_pid = task->parent->pid;
        next_task = NULL;
        
        if (list_empty(&task->sibling) || task == list_last_entry(&task->sibling,struct task_struct, sibling)) {
            p->next_sibling_pid = 0;
        } else {
            next_task = list_next_entry(task, sibling);
            p->next_sibling_pid = next_task->pid;
        }
        
        if(list_empty(&task->children)) {
            p->first_child_pid = 0;
        } else {
            next_task = list_first_entry(&task->children, struct task_struct, sibling);
            p->first_child_pid = next_task->pid;
        }

        p->uid = (int64_t)__kuid_val(task_uid(task));

        while(*(task->comm + comm_idx) != '\0') {
            *(p->comm + comm_idx) = *(task->comm + comm_idx);
            ++comm_idx;
        }
        *(p->comm + comm_idx) = '\0';
        comm_idx = 0;

        ++process_cnt;
        ++p;

        if(next_task == NULL) {
            next_task = task;
            while(next_task->pid && next_task==list_last_entry(&next_task->sibling, struct task_struct, sibling))
                next_task = next_task->parent;
            if(next_task->pid) {
                task = list_next_entry(next_task, sibling);
                continue;
            }
        }
        task = next_task;
    }
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
