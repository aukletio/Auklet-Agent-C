/* Module agent implements an Auklet agent for C or C++ programs. */

#include "timer.h"

#include <pthread.h>
#include <stdint.h>
#include "node.h"

#include "socket.h"

#include <unistd.h>
#include <signal.h>

#include "buf.h"
#include "json.h"

#include <stdlib.h>
#include <sys/time.h>

#define SAMPLE_PERIOD {.tv_sec = 0, .tv_usec = 10000}

enum {
	OFF,
	ON,
};

static void sigprof(int n);
static void sigerr(int n);
static void profilefunc();
static void setagentstate(int state);
static void __attribute__ ((constructor (101))) setup();
static void __attribute__ ((destructor (101))) cleanup();

/* global variables */

/* root is the root node of the profile tree, which does not correspond to an
 * actual callframe. Its callees, however, consist of all instrumented top-level
 * functions in the program, which could include main, any functions launched
 * with pthread_create, and possibly constructors or destructors (invoked by the
 * C runtime). */
static Node root = emptyNode;

/* sp is the stack pointer for the current thread. Since this variable is
 * declared as thread-specific, the C runtime will automatically allocate an
 * instance initialized to &root for every thread launched. */
static __thread Node *sp = &root;

/* sockfd is the file descriptor for the connection to the Auklet client. */
static int sockfd = 0;

/* agentstate allows us to enable or disable the effects of instrumentation,
 * even though there is no way to prevent the instrument functions from being
 * called. */
static int agentstate = OFF;

/* exported functions */

void
__cyg_profile_func_enter(void *fn, void *cs)
{
	if (agentstate == OFF)
		return;
	if (!push(&sp, &(Frame){(uintptr_t)fn, (uintptr_t)cs})) {
		logprint(sockfd, FATAL, "push failed");
		setagentstate(OFF);
	}
}

void
__cyg_profile_func_exit(void *fn, void *cs)
{
	if (agentstate == OFF)
		return;
	if (!pop(&sp)) {
		logprint(sockfd, FATAL, "pop failed");
		setagentstate(OFF);
	}
}

/* private functions */

/* sigprof handles SIGPROF by sampling the profile tree in the current thread. */
void
sigprof(int n)
{
	sample(sp);
}

/* sigerr handles SIGSEGV, SIGILL, and SIGFPE by emitting a stack trace and
 * exiting. */
void
sigerr(int n)
{
	sendstacktrace(sockfd, sp, n);
	_exit(1);
}

/* profilefunc calls sendprofile. It is executed when the timer expires. */
void
profilefunc()
{
	sendprofile(sockfd, &root);
}

/* setagentstate turns the agent on or off. */
void
setagentstate(int state)
{
	struct itimerval sigproftimer = {SAMPLE_PERIOD, SAMPLE_PERIOD};
	struct itimerval stop = {{0, 0}, {0, 0}};
	if (agentstate == state)
		return;
	switch (state) {
	case ON:
		siginstall(SIGPROF, sigprof);
		siginstall(SIGSEGV, sigerr);
		siginstall(SIGILL, sigerr);
		siginstall(SIGFPE, sigerr);
		setitimer(ITIMER_PROF, &sigproftimer, NULL);
		starttimer(profilefunc);
		break;
	case OFF:
		stoptimer();
		setitimer(ITIMER_PROF, &stop, NULL);
		siginstall(SIGPROF, SIG_DFL);
		siginstall(SIGSEGV, SIG_DFL);
		siginstall(SIGILL, SIG_DFL);
		siginstall(SIGFPE, SIG_DFL);
	}
	agentstate = state;
	logprint(sockfd, INFO, "agent state: %s", agentstate?"on":"off");
}

/* setup runs before main and initializes the agent. */
void
setup()
{
	sockfd = connecttoclient();
	logprint(sockfd, INFO, "Auklet Instrument version %s (%s)", AUKLET_VERSION, AUKLET_TIMESTAMP);
	setagentstate(ON);
}

/* cleanup runs after main and shuts down the agent. */
void
cleanup()
{
	setagentstate(OFF);
	freeNode(&root, 1);
	close(sockfd);
}