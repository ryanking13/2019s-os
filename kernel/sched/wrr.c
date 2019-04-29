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
// 	.enqueue_task		= enqueue_task_rt,
// 	.dequeue_task		= dequeue_task_rt,
// 	.yield_task		= yield_task_rt,
//  .yield_to_task		= yield_to_task_fair,

// 	.check_preempt_curr	= check_preempt_curr_rt,

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

// 	.prio_changed		= prio_changed_rt,
// 	.switched_to		= switched_to_rt,
//  .get_rr_interval	= get_rr_interval_fair,
// 	.update_curr		= update_curr_rt,
};