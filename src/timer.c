#include "timer.h"

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#define SIGRT(n) (SIGRTMIN + (n))
#define len(x) (sizeof(x)/sizeof(x[0]))

enum {
	REAL,
	VIRT,
};

static void sigtimer(int n);
static void settimers();
static void maketimers();
static void *functhread(void *p);

/* global variables */

static sem_t sem;

/* tmr defines two single-shot timers which trigger the execution of timerfunc.
 * We use two timers, one with a CPU clock, one with a realtime clock, so that
 * there is a maximum realtime interval between calls to timerfunc (for programs
 * that take little CPU time). The timers are reset in their corresponding
 * signal handlers, so that signals end up being delivered periodically (with a
 * variable period). */
static struct {
	clockid_t clk;
	struct timespec value;
	timer_t tid;
} tmr[] = {
	/*        clk                       value */
	[REAL] = {CLOCK_REALTIME,           {.tv_sec = 10, .tv_nsec = 0}},
	[VIRT] = {CLOCK_PROCESS_CPUTIME_ID, {.tv_sec =  1, .tv_nsec = 0}},
};

static void (*timerfunc)() = NULL;

/* exported functions */

void
starttimer(void (*func)())
{
	pthread_t t;
	timerfunc = func;
	maketimers();
	settimers();
	sem_init(&sem, 0, 0);
	siginstall(SIGRT(REAL), sigtimer);
	siginstall(SIGRT(VIRT), sigtimer);
	pthread_create(&t, NULL, functhread, NULL);
}

void
stoptimer()
{
	timerfunc = NULL;
}

/* siginstall installs handler as the signal handler for sig. */
void
siginstall(int sig, void (*handler)(int))
{
	struct sigaction sa;
	sigaction(sig, NULL, &sa);
	sa.sa_handler = handler;
	/* We fill the signal mask to prevent handler from getting interrupted
	 * by another handler. */
	sigfillset(&sa.sa_mask);
	sigaction(sig, &sa, NULL);
}

/* private functions */

/* sigtimer handles the signals delivered upon expiration of the timers.
 * Since many POSIX functions (especially mutex functions) are unsafe to call
 * from a signal handler, we use a semaphore to communicate with functhread,
 * where timerfunc can be safely executed. */
void
sigtimer(int n)
{
	settimers();
	sem_post(&sem);
}

/* functhread runs timerfunc after it acquires the semaphore. */
void *
functhread(void *p)
{
	sigset_t s;
	sigfillset(&s);
	/* We block all signals to prevent this thread from getting profiled. */
	pthread_sigmask(SIG_BLOCK, &s, NULL);
	while (1) {
		sem_wait(&sem);
		/* This function can safely use mutexes. */
		if (timerfunc)
			timerfunc();
	}
	return NULL;
}

/* settimers sets the timers to zero. */
void
settimers()
{
	for (int i = 0; i < len(tmr); ++i) {
		timer_settime(tmr[i].tid, 0, &(struct itimerspec){
			.it_interval = {0, 0},
			.it_value    = tmr[i].value,
		}, NULL);
	}
}

/* maketimers initializes the timers. */
void
maketimers()
{
	for (int i = 0; i < len(tmr); ++i) {
		timer_create(tmr[i].clk, &(struct sigevent){
			.sigev_notify = SIGEV_SIGNAL,
			.sigev_signo  = SIGRT(i),
		}, &tmr[i].tid);
	}
}
