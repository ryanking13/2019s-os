/* OS project 3 Weighted Round-Robin scheduler */

#include "sched.h"

// default timeslice is 10ms
// #define WRR_TIMESLICE		(10 * HZ / 1000)
#define WRR_TIMESLICE		(1000 * HZ / 1000)
// CPU ID that must be empty (arbitrarily selecte)
#define WRR_CPU_EMPTY 		0x0


unsigned int calc_wrr_timeslice(unsigned int weight) {
	return weight * WRR_TIMESLICE;
}

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

#define wrr_entity_is_task(wrr_se) (!(wrr_se)->my_q)

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

static struct sched_wrr_entity *pick_next_wrr_entity(struct rq *rq,
						   struct wrr_rq *wrr_rq)
{
	struct wrr_array *array = &wrr_rq->active;
	struct sched_wrr_entity *next = NULL;
	struct list_head *queue;

	queue = &array->queue;
	next = list_entry(queue->next, struct sched_wrr_entity, run_list);

	return next;
}

static struct task_struct *_pick_next_task_wrr(struct rq *rq)
{
	struct sched_wrr_entity *wrr_se;
	struct task_struct *p;
	struct wrr_rq *wrr_rq  = &rq->wrr;

	wrr_se = pick_next_wrr_entity(rq, wrr_rq);
	BUG_ON(!wrr_se);

	p = wrr_task_of(wrr_se);
	p->se.exec_start = rq_clock_task(rq);

	return p;
}

static struct task_struct * pick_next_task_wrr(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	struct task_struct *p;
	struct wrr_rq *wrr_rq = &rq->wrr;

	if (prev->sched_class == &wrr_sched_class)
		update_curr_wrr(rq);

	if (!wrr_rq->wrr_queued)
		return NULL;

	put_prev_task(rq, prev);

	p = _pick_next_task_wrr(rq);

	return p;
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{
	update_curr_wrr(rq);
}

static DEFINE_PER_CPU(cpumask_var_t, local_cpu_mask);
static int find_lowest_rq_wrr(struct task_struct *task)
{
	struct sched_domain *sd;
	struct cpumask *lowest_mask = this_cpu_cpumask_var_ptr(local_cpu_mask);
	int this_cpu = smp_processor_id();
	int cpu      = task_cpu(task);

	/* Make sure the mask is initialized first */
	if (unlikely(!lowest_mask))
		return -1;

	if (task->nr_cpus_allowed == 1)
		return -1; /* No other targets possible */

	// Remove WRR_CPU_EMPTY from cpumask
	cpumask_clear_cpu(WRR_CPU_EMPTY, lowest_mask);

	if (!cpupri_find(&task_rq(task)->rd->cpupri, task, lowest_mask))
		return -1; /* No targets found */

	/*
	 * At this point we have built a mask of cpus representing the
	 * lowest priority tasks in the system.  Now we want to elect
	 * the best one based on our affinity and topology.
	 *
	 * We prioritize the last cpu that the task executed on since
	 * it is most likely cache-hot in that location.
	 */
	if (cpumask_test_cpu(cpu, lowest_mask))
		return cpu;

	/*
	 * Otherwise, we consult the sched_domains span maps to figure
	 * out which cpu is logically closest to our hot cache data.
	 */
	if (!cpumask_test_cpu(this_cpu, lowest_mask))
		this_cpu = -1; /* Skip this_cpu opt if not among lowest */

	rcu_read_lock();
	for_each_domain(cpu, sd) {
		if (sd->flags & SD_WAKE_AFFINE) {
			int best_cpu;

			/*
			 * "this_cpu" is cheaper to preempt than a
			 * remote processor.
			 */
			if (this_cpu != -1 &&
			    cpumask_test_cpu(this_cpu, sched_domain_span(sd))) {
				rcu_read_unlock();
				return this_cpu;
			}

			best_cpu = cpumask_first_and(lowest_mask,
						     sched_domain_span(sd));
			if (best_cpu < nr_cpu_ids) {
				rcu_read_unlock();
				return best_cpu;
			}
		}
	}
	rcu_read_unlock();

	/*
	 * And finally, if there were no matches within the domains
	 * just give the caller *something* to work with from the compatible
	 * locations.
	 */
	if (this_cpu != -1)
		return this_cpu;

	cpu = cpumask_any(lowest_mask);
	if (cpu < nr_cpu_ids)
		return cpu;
	return -1;
}

static int select_task_rq_wrr(struct task_struct *p, int cpu, int sd_flag, int flags)
{
	int target;

	/* For anything but wake ups, just return the task_cpu */
	// if (sd_flag != SD_BALANCE_WAKE && sd_flag != SD_BALANCE_FORK)
	// 	goto out;

	/* OS Project 3 : one cpu (WRR_CPU_EMPTY) must be empty */
	
	if (!cpu == WRR_CPU_EMPTY)
		goto out;
	
	target = find_lowest_rq_wrr(p);

	if (target != -1) {
		cpu = target;
	} else {
		// error case, should not happen?
	}
	rcu_read_unlock();

out:
	return cpu;
}

// if newly added process is not running
// try reschedule without checking priority (not very efficient)
static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
	if (task_on_rq_queued(p) && rq->curr != p) {
		resched_curr(rq);
	}
}


static void task_fork_wrr(struct task_struct *p)
{
	/* OS Project 3 */
	INIT_LIST_HEAD(&p->wrr.run_list);
	p->wrr.time_slice	= calc_wrr_timeslice(p->wrr.weight);
	p->wrr.on_rq = 0;
}

static void set_curr_task_wrr(struct rq *rq)
{
	struct task_struct *p = rq->curr;

	p->se.exec_start = rq_clock_task(rq);
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	// TODO: load balancing here??

	struct sched_wrr_entity *wrr_se = &p->wrr;

	update_curr_wrr(rq);

	if (--p->wrr.time_slice)
		return;

	p->wrr.time_slice = calc_wrr_timeslice(p->wrr.weight);

	if (wrr_se->run_list.prev != wrr_se->run_list.next) {
		requeue_task_wrr(rq, p);
		resched_curr(rq);
		return;
	}
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