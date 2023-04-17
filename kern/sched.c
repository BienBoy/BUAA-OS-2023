#include <env.h>
#include <pmap.h>
#include <printk.h>

/* Overview:
 *   Implement a round-robin scheduling to select a runnable env and schedule it using 'env_run'.
 *
 * Post-Condition:
 *   If 'yield' is set (non-zero), 'curenv' should not be scheduled again unless it is the only
 *   runnable env.
 *
 * Hints:
 *   1. The variable 'count' used for counting slices should be defined as 'static'.
 *   2. Use variable 'env_sched_list', which contains and only contains all runnable envs.
 *   3. You shouldn't use any 'return' statement because this function is 'noreturn'.
 */
void schedule(int yield) {
	static int count = 0; // remaining time slices of current env
	struct Env *e = curenv;
	static int user_time[5] = {0}; // 创建一个用户累计运行时间片数数组

	/* We always decrease the 'count' by 1.
	 *
	 * If 'yield' is set, or 'count' has been decreased to 0, or 'e' (previous 'curenv') is
	 * 'NULL', or 'e' is not runnable, then we pick up a new env from 'env_sched_list' (list of
	 * all runnable envs), set 'count' to its priority, and schedule it with 'env_run'. **Panic
	 * if that list is empty**.
	 *
	 * (Note that if 'e' is still a runnable env, we should move it to the tail of
	 * 'env_sched_list' before picking up another env from its head, or we will schedule the
	 * head env repeatedly.)
	 *
	 * Otherwise, we simply schedule 'e' again.
	 *
	 * You may want to use macros below:
	 *   'TAILQ_FIRST', 'TAILQ_REMOVE', 'TAILQ_INSERT_TAIL'
	 */
	/* Exercise 3.12: Your code here. */
	int user_with_runnable_env[5] = { 0 };
	struct Env* i;
	TAILQ_FOREACH(i, &env_sched_list, env_sched_link) {
		user_with_runnable_env[i->env_user] = 1;
	}
	if (!e || !count || e->env_status != ENV_RUNNABLE || yield) {
		if (e && e->env_status == ENV_RUNNABLE){
			TAILQ_REMOVE(&env_sched_list, e, env_sched_link);
			TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);
			user_time[e->env_user] += e->env_pri;
		}	
		if (TAILQ_EMPTY(&env_sched_list))
			panic("schedule: no runnable envs");
		int user_to_choose = -1, min = 1<<30;
		for (int j = 0; j < 5; j++) {
			if (user_with_runnable_env[j] && user_time[j] < min) {
				min = user_time[j];
				user_to_choose = j;
			}
		}
		TAILQ_FOREACH(i, &env_sched_list, env_sched_link) {                      
	                 if (i->env_user == user_to_choose) {
				 e = i;
				 break;
			 }
        	} 
		count = e->env_pri;
	}

	count--;
	env_run(e);
}
