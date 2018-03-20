/* Module agent implements an Auklet agent for C or C++ programs. */

#include "emit.h"

#include <pthread.h>
#include <stdint.h>
#include "node.h"

#include "socket.h"

#include <unistd.h>
#include <signal.h>

#include "buf.h"
#include "json.h"

#include <stdlib.h>

static void __attribute__ ((constructor (101))) setup();
static void emitprofile();
static void emitstacktrace();
static void sigprof(int n);
static void sigerr(int n);

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

/* sock is the file descriptor for the connection to the Auklet client. */
static int sock;

/* exported functions */

void
__cyg_profile_func_enter(void *fn, void *cs)
{
}

void
__cyg_profile_func_exit(void *fn, void *cs)
{
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
	emitstacktrace(n);
	_exit(1);
}

void
emitstacktrace(int sig)
{
	Buf b = emptyBuf;
	append(&b, "{\"type\":\"event\",\"data\":");
	marshalstack(&b, sp, sig);
	append(&b, "}\n");
	write(sockfd, b.buf, b.len);
	free(b.buf);
}

void
emitprofile()
{
	Buf b = emptyBuf;
	append(&b, "{\"type\":\"profile\",\"data\":{\"tree\":");
	marshaltree(&b, &root);
	clearcounters(&root);
	write(sockfd, b.buf, b.len);
	free(b.buf);
}

/* setup runs before main and initializes the agent. */
void
setup()
{
	sock = connecttoclient();
	logprint(INFO, "Auklet Instrument version %s (%s)", AUKLET_VERSION, AUKLET_TIMESTAMP);
	siginstall(SIGPROF, sigprof);
	siginstall(SIGSEGV, sigerr);
	siginstall(SIGILL, sigerr);
	siginstall(SIGFPE, sigerr);
	emitfunc = emitprofile;
	startemitter();
}
