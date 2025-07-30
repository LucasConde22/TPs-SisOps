#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

#define RESTARTING_NUMBER 20

// Array to store the execution history of environments and the number of times
// the scheduler was called.
#define MAX_HISTORY 100
static envid_t execution_history[MAX_HISTORY];
static int history_index = 0;
static int sched_calls = 0;


void sched_halt(void);
void
sched_round_robin()
{
	struct Env *next_env = NULL;
	if (!curenv) {
		for (int i = 0; i < NENV; i++) {
			if (envs[i].env_status == ENV_RUNNABLE) {
				next_env = &envs[i];
				break;
			}
		}
	} else {
		int index = ENVX(curenv->env_id) + 1;
		while (curenv != &envs[index]) {
			if (envs[index].env_status == ENV_RUNNABLE) {
				next_env = &envs[index];
				break;
			}
			index++;
			if (index == NENV)
				index = 0;
		}
	}
	if (next_env) {
		env_run(next_env);
	} else if (curenv && curenv->env_status == ENV_RUNNING) {
		env_run(curenv);
	} else {
		sched_halt();
	}
}

void
improve_priorities()
{
	for (int i = 0; i < NENV; i++) {
		if (envs[i].env_status == ENV_RUNNABLE &&
		    envs[i].env_priority + 5 < INITIAL_PRIORITY) {
			envs[i].env_priority += 5;
		}
	}
}

void
sched_with_priority()
{
	sched_calls++;

	if (sched_calls % RESTARTING_NUMBER == 0) {
		improve_priorities();
	}

	struct Env *next_env = NULL;
	int highest_priority = -1;
	int start_index = curenv ? ENVX(curenv->env_id) + 1 : 0;

	for (int i = 0; i < NENV; i++) {
		int index = (start_index + i) % NENV;

		if (envs[index].env_status != ENV_RUNNABLE) {
			continue;
		}

		if (envs[index].env_priority > highest_priority) {
			highest_priority = envs[index].env_priority;
			next_env = &envs[index];
		} else if (envs[index].env_priority == highest_priority) {
			if (next_env == NULL ||
			    envs[index].env_runs < next_env->env_runs) {
				next_env = &envs[index];
			}
		}
	}

	if (next_env) {
		if (history_index < MAX_HISTORY) {
			execution_history[history_index++] = next_env->env_id;
		}
		if (next_env->env_priority > 0) {
			next_env->env_priority--;
		}
		env_run(next_env);
	} else if (curenv && curenv->env_status == ENV_RUNNING) {
		env_run(curenv);
	} else {
		sched_halt();
	}
}

void
sched_yield(void)
{
#ifdef SCHED_ROUND_ROBIN
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running. Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// Your code here - Round robin
	sched_round_robin();
#endif

#ifdef SCHED_PRIORITIES
	// Implement simple priorities scheduling.
	//
	// Environments now have a "priority" so it must be consider
	// when the selection is performed.
	//
	// Be careful to not fall in "starvation" such that only one
	// environment is selected and run every time.

	// Your code here - Priorities
	sched_with_priority();
#endif

	// Without scheduler, keep runing the last environment while it exists
	if (curenv) {
		env_run(curenv);
	}

	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("Scheduler statistics:\n");

		// number of times the scheduler was called
		cprintf("Total scheduler calls: %d\n", sched_calls);
		for (int j = 0; j < history_index; j++) {
			cprintf("Env %08x\n", execution_history[j]);
		}

		// number of times each env was executed
		cprintf("Execution history:\n");
		for (int j = 0; j < NENV; j++) {
			if (envs[j].env_runs > 0) {
				cprintf("Env %08x executed %d times\n",
				        envs[j].env_id,
				        envs[j].env_runs);
			}
		}
		while (1)
			monitor(NULL);
	}


	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
