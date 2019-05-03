/* OS project 3 Weighted Round-Robin scheduler */

#include "sched.h"

static void do_load_balance(void);
static int select_task_rq_wrr(struct task_struct *p, int cpu, int sd_flag, int flags);

unsigned int calc_wrr_timeslice(unsigned int weight) {
	return weight * WRR_TIMESLICE;
}

/* OS Project 3 */
void init_wrr_rq(struct wrr_rq *wrr_rq) {
	struct wrr_array *array;
	printk(KERN_INFO "Init WRR run queue\n");

	array = &wrr_rq->active;
	INIT_LIST_HEAD(&array->queue);
	// INIT_LIST_HEAD(&wrr_rq->pending_tasks);
	
	wrr_rq->wrr_nr_running = 0;
	wrr_rq->weight_sum = 0;
	wrr_rq->load_balance_time = WRR_LOAD_BALANCE_PERIOD;
	// wrr_rq->wrr_queued = 0;
	raw_spin_lock_init(&wrr_rq->wrr_runtime_lock);
}

/*
 * Trigger the SCHED_SOFTIRQ if it is time to do periodic load balancing.
 */
void trigger_load_balance_wrr(struct rq *rq)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	int cpu = smp_processor_id();

	if(--wrr_rq->load_balance_time) {
		return;
	}

	wrr_rq->load_balance_time = WRR_LOAD_BALANCE_PERIOD;

	// only one cpu must do load balancing
	if (cpu == cpumask_first(cpu_online_mask)) {
		// printk(KERN_INFO "do load balance on cpu %d\n", cpu);
		do_load_balance();
	}
}

void update_weight_wrr(struct wrr_rq *wrr_rq, struct sched_wrr_entity *wrr_se, int weight) {
	int prev_weight = wrr_se->weight;
	wrr_se->weight = weight;
	wrr_rq->weight_sum += (weight - prev_weight);
}

static inline struct rq *rq_of_wrr_rq(struct wrr_rq *wrr_rq)
{
	return wrr_rq->rq;
}

static inline int on_wrr_rq(struct sched_wrr_entity *wrr_se)
{
	return wrr_se->on_rq;
}

static inline struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_se)
{
	return container_of(wrr_se, struct task_struct, wrr);
}

/*
 * Update the current task's runtime statistics. Skip current tasks that
 * are not in our scheduling class.
 */
static void update_curr_wrr(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	// struct sched_wrr_entity *wrr_se = &curr->wrr;
	u64 delta_exec;

	if (curr->sched_class != &wrr_sched_class)
		return;

	delta_exec = rq_clock_task(rq) - curr->se.exec_start;
	if (unlikely((s64)delta_exec <= 0))
		return;

	/* Kick cpufreq (see the comment in kernel/sched/sched.h). */
	cpufreq_update_util(rq, SCHED_CPUFREQ_RT);

	schedstat_set(curr->se.statistics.exec_max,
		      max(curr->se.statistics.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq_clock_task(rq);
	cpuacct_charge(curr, delta_exec);
}

static void __enqueue_wrr_entity(struct rq *rq, struct sched_wrr_entity *wrr_se, unsigned int flags)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct wrr_array *array = &wrr_rq->active;
	struct list_head *queue = &array->queue;

	list_add_tail(&wrr_se->run_list, queue);
	wrr_se->on_rq = 1;
	// inc_rt_tasks(rt_se, rt_rq);
	wrr_rq->wrr_nr_running += 1;
	wrr_rq->weight_sum += wrr_se->weight;
	add_nr_running(rq, 1);
}

static void enqueue_wrr_entity(struct rq *rq, struct sched_wrr_entity *wrr_se, unsigned int flags)
{
	__enqueue_wrr_entity(rq, wrr_se, flags);
}


static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;
	// int cpu = cpu_of(rq);
	printk(KERN_INFO "Enqueue WRR run queue %d\n", p->pid);

	// if (!task_current(rq, p) && cpu == WRR_CPU_EMPTY) {
	// 	struct wrr_rq *wrr_rq = &rq->wrr;
	// 	list_add_tail(&p->pending_tasks, &wrr_rq->pending_tasks);
	// 	printk(KERN_INFO "%d in %d added to pending tasks\n", p->pid, cpu);
	// 	return;
	// }

	enqueue_wrr_entity(rq, wrr_se, flags);
}

static void __dequeue_wrr_entity(struct rq *rq, struct sched_wrr_entity *wrr_se, unsigned int flags)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	// struct wrr_array *array = &wrr_rq->active;

	// __delist_wrr_entity(wrr_se, array);
	list_del_init(&wrr_se->run_list);

	wrr_se->on_rq = 0;

	// dec_rt_tasks(rt_se, rt_rq);
	wrr_rq->wrr_nr_running -= 1;
	wrr_rq->weight_sum -= wrr_se->weight;
	sub_nr_running(rq, 1);
}

static void dequeue_wrr_entity(struct rq *rq, struct sched_wrr_entity *wrr_se, unsigned int flags)
{

	// dequeue_rt_stack(rt_se, flags);
	if(on_wrr_rq(wrr_se))
		__dequeue_wrr_entity(rq, wrr_se, flags);
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;
	printk(KERN_INFO "Dequeue WRR %d\n", p->pid);

	update_curr_wrr(rq);
	dequeue_wrr_entity(rq, wrr_se, flags);
}

/*
 * Put task to the head or the end of the run list without the overhead of
 * dequeue followed by enqueue.
 */
static void requeue_wrr_entity(struct wrr_rq *wrr_rq, struct sched_wrr_entity *wrr_se)
{

	if (on_wrr_rq(wrr_se)) {
		struct wrr_array *array = &wrr_rq->active;
		struct list_head *queue = &array->queue;
		list_move_tail(&wrr_se->run_list, queue);
	}
}

static void requeue_task_wrr(struct rq *rq, struct task_struct *p)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;
	struct wrr_rq *wrr_rq;
	
	// printk(KERN_INFO "Requeue WRR %d\n", p->pid);

	wrr_rq = &rq->wrr;
	requeue_wrr_entity(wrr_rq, wrr_se);
}

static void yield_task_wrr(struct rq *rq)
{
	printk(KERN_INFO "Yield WRR\n");
	requeue_task_wrr(rq, rq->curr);
}

static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	// no preemption for WRR;
	printk(KERN_INFO "Check preempt WRR %d\n", p->pid);
	resched_curr(rq);
	return;
}

static void prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{
	// priority have no effect for WRR;
	printk(KERN_INFO "Prio changed WRR %d\n", p->pid);
	return;
}

static struct sched_wrr_entity *pick_next_wrr_entity(struct rq *rq,
						   struct wrr_rq *wrr_rq)
{
	struct wrr_array *array = &wrr_rq->active;
	struct sched_wrr_entity *next = NULL;
	struct list_head *queue;

	queue = &array->queue;

	if (list_empty_careful(queue->next)) {
		return NULL;
	}

	next = list_entry(queue->next, struct sched_wrr_entity, run_list);

	return next;
}

static struct task_struct *_pick_next_task_wrr(struct rq *rq)
{
	struct sched_wrr_entity *wrr_se;
	struct task_struct *p;
	struct wrr_rq *wrr_rq  = &rq->wrr;
	struct cfs_rq *cfs_rq = &rq->cfs;

	wrr_se = pick_next_wrr_entity(rq, wrr_rq);

	if (wrr_se == NULL) {
		return NULL;
	}

	// BUG_ON(!wrr_se);
	// to make CFS process pass through while WRR processes are active,
	// reserve some time slice for CFS process
	if (wrr_se->time_slice < WRR_TIMESLICE_BANDWIDTH) {
		// time slice is in reserved range
		return NULL;
	}

	p = wrr_task_of(wrr_se);
	p->se.exec_start = rq_clock_task(rq);

	return p;
}

static struct task_struct * pick_next_task_wrr(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	struct task_struct *p;
	// struct wrr_rq *wrr_rq = &rq->wrr;

	// printk(KERN_INFO "Pick next task WRR\n");

	if (prev->sched_class == &wrr_sched_class)
		update_curr_wrr(rq);

	// if (!wrr_rq->wrr_queued)
	// 	return NULL;


	put_prev_task(rq, prev);

	p = _pick_next_task_wrr(rq);

	return p;
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{
	// printk(KERN_INFO "Put prev task WRR %d\n", p->pid);
	update_curr_wrr(rq);
}

static int select_task_rq_wrr(struct task_struct *p, int cpu, int sd_flag, int flags)
{
	int cpui;
	int target_cpu = cpu; // Note : this can be WRR_CPU_EMPTY, but it must be, since a process should run anyway.
	int target_weight_sum = INT_MAX;

	/* For anything but wake ups, just return the task_cpu */
	// if (sd_flag != SD_BALANCE_WAKE && sd_flag != SD_BALANCE_FORK)
	// 	goto out;

	/* OS Project 3 : one cpu (WRR_CPU_EMPTY) must be empty */
	rcu_read_lock();

	// https://stackoverflow.com/questions/24437724/diff-between-various-cpu-masks-linux-kernel
	for_each_online_cpu(cpui) {
		struct rq *rq = cpu_rq(cpui);
		struct wrr_rq *wrr_rq = &rq->wrr;

		if (cpui == WRR_CPU_EMPTY) continue;

		if (cpumask_test_cpu(cpui, &p->cpus_allowed)) {
			if(wrr_rq->weight_sum < target_weight_sum) {
				target_weight_sum = wrr_rq->weight_sum;
				target_cpu = cpui;
			}
		}
	}
	rcu_read_unlock();
	printk(KERN_INFO "Select task rq WRR p:%d cpu: %d\n", p->pid, target_cpu);

	return target_cpu;
}

// if newly added process is not running
// try reschedule without checking priority (not very efficient)
static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
	printk(KERN_INFO "switched to WRR %d\n", p->pid);
	if (task_on_rq_queued(p) && rq->curr != p) {
		resched_curr(rq);
	}
}


static void task_fork_wrr(struct task_struct *p)
{
	/* OS Project 3 */
	printk(KERN_INFO "fork WRR %d\n", p->pid);
	// INIT_LIST_HEAD(&p->wrr.run_list);
	// p->wrr.time_slice	= calc_wrr_timeslice(p->wrr.weight);
	// p->wrr.on_rq = 0;
	return;
}

static void set_curr_task_wrr(struct rq *rq)
{
	struct task_struct *p = rq->curr;
	printk(KERN_INFO "Set curr task WRR\n");

	p->se.exec_start = rq_clock_task(rq);
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	// TODO: load balancing here??

	// struct sched_wrr_entity *wrr_se = &p->wrr;
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct wrr_array *array = &wrr_rq->active;
	struct list_head *queue = &array->queue;

	update_curr_wrr(rq);

	// printk(KERN_INFO "TIMESLICE %d %d\n", p->pid, p->wrr.time_slice);

	// TODO: delete this code,
	// must not happen case
	// if (p->wrr.time_slice <= 0) {
	// 	printk(KERN_INFO "time slice is negative pid: %d\n", p->pid);
	// }

	if (--p->wrr.time_slice)
		return;

	p->wrr.time_slice = calc_wrr_timeslice(p->wrr.weight);

	// printk(KERN_INFO "tick rotation: %d %d\n", p->pid, p->wrr.time_slice);

	if (queue->prev != queue->next) {
		// printk(KERN_INFO "try reschedule WRR\n");
		requeue_task_wrr(rq, p);
		resched_curr(rq);
		return;
	}
}

static void do_load_balance(void) {
	int cpui;
	int max_cpu = -1;
	int min_cpu = -1;
	struct rq *max_rq;
	struct rq *min_rq;
	int min_ws = INT_MAX;
	int max_ws = -1;
	struct wrr_rq *max_wrr_rq;
	struct wrr_rq *min_wrr_rq;
	struct list_head *head;
	struct sched_wrr_entity *wrr_se;
	struct sched_wrr_entity *migrate_target = NULL;
	int migrate_weight = -1;
	unsigned long flags;

	rcu_read_lock();
	for_each_online_cpu(cpui) {
		struct rq *rq = cpu_rq(cpui);
		struct wrr_rq *wrr_rq = &rq->wrr;
		int ws = wrr_rq->weight_sum;
		// TODO: remove this
		// printk("[lb] cpu: %d / ws: %d\n", cpui, ws);
		// head = &wrr_rq->active.queue;
		// list_for_each_entry(wrr_se, head, run_list) {
		// 	printk("[lb process] cpu: %d, pid: %d\n", cpui, wrr_task_of(wrr_se)->pid);
		// }

		// WRR_CPU_EMPTY cannot participate
		if (cpui == WRR_CPU_EMPTY) continue;

		if (ws > max_ws) {
			max_ws = ws;
			max_cpu = cpui;
		}

		if (ws < min_ws) {
			min_ws = ws;
			min_cpu = cpui;
		}
	}
	rcu_read_unlock();

	// nothing to move
	if (max_cpu == min_cpu) return;

	max_rq = cpu_rq(max_cpu);
	min_rq = cpu_rq(min_cpu);


	// https://enginius.tistory.com/107
	local_irq_save(flags);
	double_rq_lock(max_rq, min_rq);

	max_wrr_rq = &max_rq->wrr;
	min_wrr_rq = &min_rq->wrr;
	head = &max_wrr_rq->active.queue;
	list_for_each_entry(wrr_se, head, run_list) {
		int weight = wrr_se->weight;
		struct task_struct *p = wrr_task_of(wrr_se);

		// if p is currently running, cannot migrate
		if (max_rq->curr == p) continue;

		// if p is not allowed to run in min_cpu, cannot migrate
		if (!cpumask_test_cpu(min_cpu, &p->cpus_allowed)) continue;

		// weight sum should not reversed after migration
		if (max_ws - weight <= min_ws + weight) continue;

		if (weight > migrate_weight) {
			migrate_weight = weight;
			migrate_target = wrr_se;
		}
	}

	if (migrate_target != NULL) {
		struct task_struct *p = wrr_task_of(migrate_target);
		printk(KERN_INFO "moving %d from cpu %d to %d\n", p->pid, max_cpu, min_cpu);
		// detach_task() in fair.c
		p->on_rq = TASK_ON_RQ_MIGRATING;
		deactivate_task(max_rq, p, DEQUEUE_NOCLOCK);
		set_task_cpu(p, min_cpu);

		// attach_task() in fair.c
		activate_task(min_rq, p, ENQUEUE_NOCLOCK);
		p->on_rq = TASK_ON_RQ_QUEUED;
		// check_preempt_curr(min_rq, p, 0);
	}

	double_rq_unlock(max_rq, min_rq);
	local_irq_restore(flags);
	return;
}

const struct sched_class wrr_sched_class = {
    // rt -> wrr -> cfs
	.next			= &fair_sched_class,
	.enqueue_task		= enqueue_task_wrr,
	.dequeue_task		= dequeue_task_wrr,
	.yield_task		= yield_task_wrr,
//  .yield_to_task		= yield_to_task_fair, // no need to implement?

	.check_preempt_curr	= check_preempt_curr_wrr,

	.pick_next_task		= pick_next_task_wrr,
	.put_prev_task		= put_prev_task_wrr,

// #ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_wrr,
//  .migrate_task_rq	= migrate_task_rq_fair, // no need to implement?

	.set_cpus_allowed       = set_cpus_allowed_common,
// 	.rq_online              = rq_online_rt, // no need to implement?
// 	.rq_offline             = rq_offline_rt, // no need to implement?
// 	.task_woken		= task_woken_rt, // (not sure) no need to implement?
// 	.switched_from		= switched_from_rt, // no need to implement?
// #endif

	.set_curr_task          = set_curr_task_wrr,
	.task_tick		= task_tick_wrr,
	.task_fork		= task_fork_wrr,
//  .task_dead		= task_dead_fair, // noting to handle on dead?
// 	.get_rr_interval	= get_rr_interval_rt, // no need to implement?

	.prio_changed		= prio_changed_wrr,
	.switched_to		= switched_to_wrr,
	.update_curr		= update_curr_wrr,
};