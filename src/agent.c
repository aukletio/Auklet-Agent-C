/* Module agent implements an Auklet agent for C or C++ programs. */

#include <pthread.h>
#include "server.h"

#include <stdint.h>
#include "node.h"

#include "logger.h"

#include <unistd.h>
#include "libauklet.h"

#include <signal.h>
#include "buf.h"

#include "json.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>

#define SAMPLE_PERIOD {.tv_sec = 0, .tv_usec = 10000}

enum {
	OFF,
	ON,
};

static void sigprof(int n);
static void sigerr(int n);
static void profilehandler();
static void siginstall(int sig, void (*handler)(int));
static void setagentstate(int state);
static int connecttoclient();
static void __attribute__ ((constructor (101))) setup();
static void __attribute__ ((destructor (101))) cleanup();

/* global variables */

/* root is the root node of the profile tree, which does not correspond to an
 * actual callframe. Its callees, however, consist of all instrumented top-level
 * functions in the program, which could include main, any functions launched
 * with pthread_create, and possibly constructors or destructors (invoked by the
 * C runtime). */
static Node root = emptyNode(realloc);

/* sp is the stack pointer for the current thread. Since this variable is
 * declared as thread-specific, the C runtime will automatically allocate an
 * instance initialized to &root for every thread launched. */
static __thread Node *sp = &root;

/* sockfd is the file descriptor for the connection to the Auklet client. */
static int sockfd = 0;

/* server responds to emission requests received from the client. */
static Server *server;

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

int
auklet_send(const char *data, size_t size)
{
	/* File descriptor 3 is inherited from the Client
	 * for transmitting custom JSON messages. */
	int fd = 3;
	int offset = 0;
	while (offset < size) {
		ssize_t wc = write(fd, data + offset, size - offset);
		if (-1 == wc)
			return -1;
		offset += wc;
	}
	return offset;
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
	Buf b = emptyBuf(realloc, free);
	profilehandler();
	marshalstacktrace(&b, sp, n);
	write(sockfd, b.buf, b.len); /* what if write fails? */
	b.free(b.buf);
	_exit(1);
}

/* profilehandler calls sendprofile. It is executed when the agent receives an
 * emission request. */
void
profilehandler()
{
	Buf b = emptyBuf(realloc, free);
	marshalprofile(&b, &root);
	write(sockfd, b.buf, b.len); /* what if write fails? */
	clearcounters(&root);
	b.free(b.buf);
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

/* setagentstate turns the agent on or off. */
void
setagentstate(int state)
{
	struct itimerval sigproftimer = {SAMPLE_PERIOD, SAMPLE_PERIOD};
	struct itimerval stopinterval = {{0, 0}, {0, 0}};
	if (agentstate == state)
		return;
	switch (state) {
	case ON:
		siginstall(SIGPROF, sigprof);
		siginstall(SIGSEGV, sigerr);
		siginstall(SIGILL, sigerr);
		siginstall(SIGFPE, sigerr);
		setitimer(ITIMER_PROF, &sigproftimer, NULL);
		start(server);
		break;
	case OFF:
		wait(server, 1);
		setitimer(ITIMER_PROF, &stopinterval, NULL);
		siginstall(SIGPROF, SIG_DFL);
		siginstall(SIGSEGV, SIG_DFL);
		siginstall(SIGILL, SIG_DFL);
		siginstall(SIGFPE, SIG_DFL);
	}
	agentstate = state;
	logprint(sockfd, INFO, "agent state: %s", agentstate?"on":"off");
}

/* connecttoclient connects to the socket provided by
 * an Auklet client and returns a valid file descriptor
 * if the connection succeeded, othwerise -1. */
int
connecttoclient()
{
	int fd = 4;
	struct stat buf;

	if (-1 == fstat(fd, &buf))
		return -1;
	return fd;
}

/* setup runs before main and initializes the agent. */
void
setup()
{
	int fd = connecttoclient();
	if (-1 == fd)
		/* We have no data connection. Dump to stdout. */
		sockfd = 0;
	else
		sockfd = fd;

	server = newServer(sockfd, profilehandler, malloc);
	dprintf(sockfd, "{\"version\":\"%s %s\"}\n", AUKLET_VERSION, AUKLET_TIMESTAMP);
	setagentstate(ON);
}

/* cleanup runs after main and shuts down the agent. */
void
cleanup()
{
	profilehandler();
	setagentstate(OFF);
	freeNode(&root, 1, free);
	free(server);
	close(sockfd);
}
