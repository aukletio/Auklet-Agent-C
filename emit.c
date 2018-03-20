#include "emit.h"

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

static void sigemit(int n);
static void settimers();
static void maketimers();
static void *emissionthread(void *p);

/* global variables */

static sem_t sem;

/* tmr defines two single-shot timers which trigger the emission of a profile
 * tree. We use two timers, one with a CPU clock, one with a realtime clock, so
 * that there is a maximum realtime interval between emissions (for programs
 * that take little CPU time). The timers are reset in their corresponding
 * signal handlers, which leads to signals being delivered with indefinite
 * repetition. */
static struct {
	clockid_t clk;
	struct timespec value;
	timer_t tid;
} tmr[] = {
	/*        clk                       value */
	[REAL] = {CLOCK_REALTIME,           {.tv_sec = 10, .tv_nsec = 0}},
	[VIRT] = {CLOCK_PROCESS_CPUTIME_ID, {.tv_sec =  1, .tv_nsec = 0}},
};

/* exported functions */

void
startemitter()
{
	pthread_t t;
	maketimers();
	settimers();
	sem_init(&sem, 0, 0);
	siginstall(SIGRT(REAL), sigemit);
	siginstall(SIGRT(VIRT), sigemit);
	pthread_create(&t, NULL, emissionthread, NULL);
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

/* sigemit handles the signals delivered upon expiration of the emission timers.
 * Since many POSIX functions (especially mutex functions) are unsafe to call
 * from a signal handler, we use a semaphore to communicate with emissionthread,
 * which safely emits the profile tree. */
void
sigemit(int n)
{
	settimers();
	sem_post(&sem);
}

/* emissionthread emits profile data after it acquires the semaphore. */
void *
emissionthread(void *p)
{
	sigset_t s;
	sigfillset(&s);
	/* We block all signals to prevent this thread from getting profiled. */
	pthread_sigmask(SIG_BLOCK, &s, NULL);
	while (1) {
		sem_wait(&sem);
		/* Emit the profile tree. This function can safely use mutexes. */
		emitfunc();
	}
	return NULL;
}

/* settimers sets the emission timers to zero. */
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

/* maketimers initializes the emission timers. */
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
