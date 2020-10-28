/*
 * Reproducer for BUG_ON(!newowner) in fixup_pi_state_owner()
 * <linux>/kernel/futex.c
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <err.h>
#include <errno.h>
#include <sched.h>
#include <time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <linux/futex.h>
#include <limits.h>

#define TEST_CPU	1
#define NSEC_PER_USEC	1000ULL
#define NSEC_PER_SEC	1000000000ULL

#define FUTEX_TID_MASK	0x3fffffff

typedef uint32_t futex_t;

static futex_t cond;
static futex_t lock1 = 0;
static futex_t lock2 = 0;
static pthread_barrier_t start_barrier;

static int futex_lock_pi(futex_t *futex)
{
	int ret;
	pid_t tid;
	futex_t zero = 0;

	if (!futex)
		return EINVAL;

	tid = syscall(SYS_gettid);
	if (tid == (*futex & FUTEX_TID_MASK))
		return EDEADLOCK;

	ret = __atomic_compare_exchange_n(futex, &zero, tid, false,
					  __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	/* no pending waiters; we got the lock */
	if (ret)
		return 0;

	ret = syscall(SYS_futex, futex,
		      FUTEX_LOCK_PI | FUTEX_PRIVATE_FLAG,
		      0, NULL, NULL, 0);
	return (ret) ? errno : 0;
}

static int futex_unlock_pi(futex_t *futex)
{
	int ret;
	pid_t tid;

	if (!futex)
		return EINVAL;

	tid = syscall(SYS_gettid);
	if (tid != (*futex & FUTEX_TID_MASK))
		return EPERM;

	ret = __atomic_compare_exchange_n(futex, &tid, 0, false,
					  __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	if (ret)
		return 0;

	ret = syscall(SYS_futex, futex,
		      FUTEX_UNLOCK_PI | FUTEX_PRIVATE_FLAG,
		      0, NULL, NULL, 0);
	return (ret) ? errno : 0;
}

static int futex_wait_requeue_pi(futex_t *cond, futex_t *futex,
				 const struct timespec *to)
{
	int ret;

	(*cond)++;
	futex_unlock_pi(futex);

	ret = syscall(SYS_futex, cond,
		      FUTEX_WAIT_REQUEUE_PI | FUTEX_PRIVATE_FLAG,
		      *cond, to, futex, 0);
	if (ret) {
		ret = errno;
		/* on error, the kernel didn't acquire the lock for us */
		futex_lock_pi(futex);
	}

	return ret;
}

static int futex_cmp_requeue_pi(futex_t *cond, futex_t *futex)
{
	int ret;

	(*cond)++;
	ret = syscall(SYS_futex, cond,
		      FUTEX_CMP_REQUEUE_PI | FUTEX_PRIVATE_FLAG,
		      1, INT_MAX, futex, *cond);

	return (ret) ? errno : 0;
}

static void tsnorm(struct timespec *ts)
{
	while (ts->tv_nsec >= NSEC_PER_SEC) {
		ts->tv_nsec -= NSEC_PER_SEC;
		ts->tv_sec++;
	}
}

static unsigned long long tsdiff(struct timespec *start, struct timespec *end)
{
	unsigned long long t1 = (unsigned long long)(start->tv_sec) * NSEC_PER_SEC +
		start->tv_nsec;
	unsigned long long t2 = (unsigned long long)(end->tv_sec) * NSEC_PER_SEC +
		end->tv_nsec;

	return t2 - t1;
}

static void set_cpu_affinity(int cpu)
{
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask) < 0)
		err(1, "Failed to set the CPU affinity");
}

static void clr_cpu_affinity()
{
	cpu_set_t mask;
	int i;

	CPU_ZERO(&mask);
	for (i = 0; i < get_nprocs(); i++)
		CPU_SET(i, &mask);

	if (sched_setaffinity(0, sizeof(mask), &mask) < 0)
		err(1, "Failed to clear the CPU affinity");
}

static void set_fifo_priority(int prio)
{
	struct sched_param schedp;

	memset(&schedp, 0, sizeof(schedp));
	schedp.sched_priority = prio;
	if (sched_setscheduler(0, SCHED_FIFO, &schedp) < 0)
		err(1, "Failed to set the test thread priority");
}

static void do_busy_work(unsigned long long nsec)
{
	struct timespec start_ts;
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &start_ts);
	do {
		clock_gettime(CLOCK_MONOTONIC, &ts);
	} while (tsdiff(&start_ts, &ts) < nsec);
}


static void* boosted_thread(void *param)
{
	struct timespec sleep_ts = {.tv_sec = 0, .tv_nsec = 600000ULL};

	while (1)
	{
		set_cpu_affinity(TEST_CPU);
		pthread_barrier_wait(&start_barrier);
		clr_cpu_affinity();

		futex_lock_pi(&lock2);
		clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_ts, NULL);
		futex_lock_pi(&lock1);
		futex_unlock_pi(&lock1);
		futex_unlock_pi(&lock2);
	}
}

#define SWEEP_START	500000ULL
#define SWEEP_END	900000ULL
#define SWEEP_STEP	100

static void* rt_thread(void *param)
{
	struct timespec sleep_ts;
	unsigned long long nsec;

	set_cpu_affinity(TEST_CPU);
	set_fifo_priority(7);

	nsec = SWEEP_START;
	while(1) {
		pthread_barrier_wait(&start_barrier);
		nsec += SWEEP_STEP;
		if (nsec > SWEEP_END)
			nsec = SWEEP_START;
		sleep_ts.tv_sec = 0;
		sleep_ts.tv_nsec = nsec;
		clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_ts, NULL);

		/* preempt the waiter thread right after it got
		 * signaled and allow the boosted_thread to block on
		 * lock1 after taking lock2 */
		do_busy_work(2000000ULL);

		/* this should boost the boosted_thread and migrate it */
		futex_lock_pi(&lock2);
		futex_unlock_pi(&lock2);
	}
}

static void* waiter_thread(void *param)
{
	struct timespec ts;

	set_cpu_affinity(TEST_CPU);

	while(1) {
		pthread_barrier_wait(&start_barrier);

		futex_lock_pi(&lock1);
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ts.tv_nsec += 800000ULL;
		tsnorm(&ts);
		futex_wait_requeue_pi(&cond, &lock1, &ts);
		futex_unlock_pi(&lock1);
	}
}

static void* waker_thread(void *param)
{
	struct timespec sleep_ts = {.tv_sec = 0, .tv_nsec = 500000ULL};

	set_cpu_affinity(TEST_CPU);

	while(1) {
		pthread_barrier_wait(&start_barrier);
		clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_ts, NULL);

		futex_lock_pi(&lock1);
		futex_cmp_requeue_pi(&cond, &lock1);
		futex_unlock_pi(&lock1);
	}
}

int main(int argc, char *argv[])
{
	pthread_t waiter;
	pthread_t waker;
	pthread_t rt;
	pthread_t boosted;

	if (pthread_barrier_init(&start_barrier, NULL, 4))
		err(1, "Failed to create start_barrier");

	if (pthread_create(&waker, NULL, waker_thread, NULL) != 0)
		err(1, "Failed to create waker thread");
	pthread_setname_np(waker, "f_waker");

	if (pthread_create(&waiter, NULL, waiter_thread, NULL) != 0)
		err(1, "Failed to create waiter thread");
	pthread_setname_np(waiter, "f_waiter");

	if (pthread_create(&rt, NULL, rt_thread, NULL) != 0)
		err(1, "Failed to create rt thread");
	pthread_setname_np(rt, "f_rt");

	if (pthread_create(&boosted, NULL, boosted_thread, NULL) != 0)
		err(1, "Failed to create boosted thread");
	pthread_setname_np(boosted, "f_boosted");

	/* block here */
	pthread_join(waker, NULL);
	pthread_join(waiter, NULL);
	pthread_join(rt, NULL);
	pthread_join(boosted, NULL);
}
