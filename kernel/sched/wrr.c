/* OS project 3 Weighted Round-Robin scheduler */

#include "sched.h"

// default timeslice is 10ms
#define WRR_TIMESLICE		(10 * HZ / 1000)

/* OS Project 3 */
void init_wrr_rq(struct wrr_rq *wrr_rq) {
	struct wrr_array *array;
	array = &wrr_rq->active;
	INIT_LIST_HEAD(&array->queue);
	
	wrr_rq->wrr_nr_running = 0;
	wrr_rq->weight_sum = 0;
	wrr_rq->wrr_queued = 0;
	raw_spin_lock_init(&wrr_rq->wrr_runtime_lock);
}

static inline struct rq *rq_of_wrr_se(struct sched_wrr_entity *wrr_se)
{
	struct wrr_rq *wrr_rq = wrr_se->wrr_rq;

	return wrr_rq->rq;
}

static inline struct rq *rq_of_wrr_rq(struct wrr_rq *wrr_rq)
{
	return wrr_rq->rq;
}

static inline struct wrr_rq *wrr_rq_of_se(struct sched_wrr_entity *wrr_se)
{
	return wrr_se->wrr_rq;
}

static inline int on_wrr_rq(struct sched_wrr_entity *wrr_se)
{
	return wrr_se->on_rq;
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

static void __enqueue_wrr_entity(struct sched_wrr_entity *wrr_se, unsigned int flags)
{
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
	struct wrr_array *array = &wrr_rq->active;
	struct list_head *queue = &array->queue;

	list_add_tail(&wrr_se->run_list, queue);
	wrr_se->on_rq = 1;
	// inc_rt_tasks(rt_se, rt_rq);
	wrr_rq->wrr_nr_running += 1;
}

static void enqueue_wrr_entity(struct sched_wrr_entity *wrr_se, unsigned int flags)
{
	// struct rq *rq = rq_of_wrr_se(wrr_se);
	__enqueue_wrr_entity(wrr_se, flags);
}


static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;
	enqueue_wrr_entity(wrr_se, flags);
}

static void __dequeue_wrr_entity(struct sched_wrr_entity *wrr_se, unsigned int flags)
{
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
	// struct wrr_array *array = &wrr_rq->active;

	// __delist_wrr_entity(wrr_se, array);
	list_del_init(&wrr_se->run_list);

	wrr_se->on_rq = 0;

	// dec_rt_tasks(rt_se, rt_rq);
	wrr_rq->wrr_nr_running -= 1;
}

static void dequeue_wrr_entity(struct sched_wrr_entity *wrr_se, unsigned int flags)
{
	// struct rq *rq = rq_of_wrr_se(wrr_se);

	// dequeue_rt_stack(rt_se, flags);
	if(on_wrr_rq(wrr_se))
		__dequeue_wrr_entity(wrr_se, flags);
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;

	update_curr_wrr(rq);
	dequeue_wrr_entity(wrr_se, flags);
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

	wrr_rq = wrr_rq_of_se(wrr_se);
	requeue_wrr_entity(wrr_rq, wrr_se);
}

static void yield_task_wrr(struct rq *rq)
{
	requeue_task_wrr(rq, rq->curr);
}

static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	// no preemption for WRR;
	return;
}

static void prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{
	// priority have no effect for WRR;
	return;
}

// /*
//  * Trigger the SCHED_SOFTIRQ if it is time to do periodic load balancing.
//  */
// void trigger_load_balance(struct rq *rq)
// {
// 	/* Don't need to rebalance while attached to NULL domain */
// 	if (unlikely(on_null_domain(rq)))
// 		return;

// 	if (time_after_eq(jiffies, rq->next_balance))
// 		raise_softirq(SCHED_SOFTIRQ);
// #ifdef CONFIG_NO_HZ_COMMON
// 	if (nohz_kick_needed(rq))
// 		nohz_balancer_kick();
// #endif
// }

// // from requeue_task_rt
// static void requeue_task_wrr(struct rq *rq, struct task_struct *p, int head)
// {
// 	struct sched_wrr_entity *wrr_se = &p->wrr;
// 	struct rt_rq *rt_rq;

// 	for_each_sched_rt_entity(rt_se) {
// 		rt_rq = rt_rq_of_se(rt_se);
// 		requeue_rt_entity(rt_rq, rt_se, head);
// 	}
// }

// // from yield_task_rt
// static void yield_task_rt(struct rq *rq)
// {
// 	requeue_task_wrr(rq, rq->curr, 0);
// }

// // from task_tick_rt
// static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
// {
// 	struct sched_wrr_entity *wrr_se = &p->wrr;
// 	// update_curr_rt(rq);

// 	if (--p->wrr.time_slice)
// 		return;

// 	p->wrr.time_slice = p->wrr.weight * WRR_BASE_TIMESLICE;

//     if (rt_se->run_list.prev != rt_se->run_list.next) {
//         requeue_task_wrr(rq, p, 0);
//         resched_curr(rq);
//         return;
//     }
// }

const struct sched_class wrr_sched_class = {
    // rt -> wrr -> cfs
	.next			= &fair_sched_class,
	.enqueue_task		= enqueue_task_wrr,
	.dequeue_task		= dequeue_task_wrr,
	.yield_task		= yield_task_wrr,
//  .yield_to_task		= yield_to_task_fair, // no need to implement?

	.check_preempt_curr	= check_preempt_curr_wrr,

// 	.pick_next_task		= pick_next_task_rt,
// 	.put_prev_task		= put_prev_task_rt,

// #ifdef CONFIG_SMP
// 	.select_task_rq		= select_task_rq_rt,
//  .migrate_task_rq	= migrate_task_rq_fair,

	.set_cpus_allowed       = set_cpus_allowed_common,
// 	.rq_online              = rq_online_rt,
// 	.rq_offline             = rq_offline_rt,
// 	.task_woken		= task_woken_rt,
// 	.switched_from		= switched_from_rt,
// #endif

// 	.set_curr_task          = set_curr_task_rt,
// 	.task_tick		= task_tick_rt,
//	.task_fork		= task_fork_fair,
//  .task_dead		= task_dead_fair,
// 	.get_rr_interval	= get_rr_interval_rt,

	.prio_changed		= prio_changed_wrr,
// 	.switched_to		= switched_to_rt,
	.update_curr		= update_curr_wrr,
};