/*
 * profiler library, separated from runtime for testing
 */

/* headers */
#include <errno.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* macros */
#define len(x) (sizeof(x)/sizeof(x[0]))

/* types */
/* Type F represents a stack frame holding function address and callsite. */
typedef struct {
	uintptr_t fn, cs;
} F;

/* Type N represents a node in a stacktree. A stacktree is an aggregation of
 * stacktraces. Stack pointers (type (N *)) should be declared thread-specific
 * (__thread). Mutex locks prefixed with l are used to avoid races between
 * instrumented threads. */
typedef struct n { 
	F f; 
	struct n *parent;

	/* lsamp guards nsamp in sample, marshal, and sane. */
	pthread_mutex_t lsamp;
	unsigned nsamp;

	/* llist guards callee, cap, and len in push. */
	pthread_mutex_t llist;
	struct n **callee;
	unsigned cap, len;

	/* lcall guards ncall in push. */
	pthread_mutex_t lcall;
	unsigned ncall;

	/* A node is empty if nsamp == ncall == 0 for itself and all of its
	 * children. */
	int empty;
} N;

/* Type B represents a resizable text buffer. */
typedef struct {
	char *buf;
	unsigned cap, len;
} B;

enum {
	DEBUG,
	INFO,
	FATAL,
};

/* function declarations */
static int push(N **sp, F f);
static int pop(N **sp);
static int eqF(F a, F b);
static N *newN(F f);
static void dumpN(int level, N *n, unsigned ind);
static void killN(N *n, int root);
static N *hascallee(N *n, F f);
static N *addcallee(N *n, F f);
static void sample(N *sp);
static void setnotempty(N *n);

static void growB(B *b);
static int append(B *b, char *fmt, ...);

static void marshal(B *b, N *n);
static void marshals(B *b, N *sp, int sig);
static int logprint(int level, char *fmt, ...);

static jmp_buf nomem;
static int log = 0; // stdout, initially
static char *loglevel[] = {
	[DEBUG] = "debug", // memory alloc
	[INFO] = "info",   // init message
	[FATAL] = "fatal", // unrecoverable error
};


/* function definitions */
#if defined(FAULT_RATE)
static void *
fault_inject(void *p)
{
	if (!p)
		return NULL;
	if (rand() < RAND_MAX/FAULT_RATE) {
		dprintf(log, "fault injected\n");
		free(p);
		errno = ENOMEM;
		return NULL;
	}
	return p;
}
#define malloc(size)       fault_inject(malloc(size))
#define realloc(ptr, size) fault_inject(realloc((ptr), (size)))
#endif

/* Push a frame f onto a stack defined by sp. Return 0 if the push failed,
 * otherwise 1. */
static int
push(N **sp, F f)
{
	pthread_mutex_lock(&(*sp)->llist);
	N *c = hascallee(*sp, f);
	if (!c) {
		c = addcallee(*sp, f);
		if (!c) {
			pthread_mutex_unlock(&(*sp)->llist);
			logprint(FATAL, "push: couldn't add callee");
			return 0;
		}
	}
	pthread_mutex_unlock(&(*sp)->llist);
	pthread_mutex_lock(&c->lcall);
	++c->ncall;
	pthread_mutex_unlock(&c->lcall);
	*sp = c;
	setnotempty(*sp);
	return 1;
}

/* Pop a frame off a stack sp. */
static int
pop(N **sp)
{
	if (!(*sp)->parent) {
		logprint(FATAL, "pop: called with NULL parent");
		return 0;
	}
	*sp = (*sp)->parent;
	return 1;
}

static int
eqF(F a, F b)
{
	return ((a.fn == b.fn) && (a.cs == b.cs));
}

/* Create a new node with frame f. */
static N *
newN(F f)
{
	N *n = (N *)malloc(sizeof(N));
	//logprint("%p = malloc(%d)", n, sizeof(N));
	if (!n)
		return NULL;

	n->f = f;
	n->ncall = n->nsamp = n->cap = n->len = 0;
	n->callee = NULL;
	n->parent = NULL;
	n->llist = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	n->lsamp = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	n->lcall = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	n->empty = 1;
	return n;
}

/* Print a node n and its callees. For debugging purposes. */
static void
dumpN(int level, N *n, unsigned ind)
{
	char tab[] = "          ";
	tab[ind] = '\0';
	logprint(level, "%s%p: fn:%p cs:%p empty:%d nsamp:%u ncall:%u len/cap:%u/%u callee:%p",
		tab,
		(void *)n,
		(void *)n->f.fn,
		(void *)n->f.cs,
		n->empty,
		n->nsamp,
		n->ncall,
		n->len,
		n->cap,
		n->callee
	);
	for (int i = 0; i < n->len; ++i)
		dumpN(level, n->callee[i], ind + 1);
}

/* Delete a node n and its callees. If root is 0, free the children of the given
 * node, but not the node itself. This allows us to avoid freeing the
 * statically-allocated root node that is required for thread-local storage
 * initialization. */
static void
killN(N *n, int root)
{
	for (int i = 0; i < n->len; ++i)
		killN(n->callee[i], 0);
	//logprint("free(%p)", n->callee);
	free(n->callee);

	if (!root) {
		//logprint("free(%p)", n);
		free(n);
	}
}

/* Return a pointer to a callee of n matching frame f, otherwise NULL. */
static N *
hascallee(N *n, F f)
{
	for (int i = 0; i < n->len; ++i)
		if (eqF(n->callee[i]->f, f))
			return n->callee[i];
	return NULL;
}

/* Add to node n a callee with frame f. If there is a memory allocation error,
 * return NULL, otherwise, a pointer to the created node. */
static N *
addcallee(N *n, F f)
{
	N *new;
	if (n->cap == n->len) {
		unsigned newcap = n->cap ? 2 * n->cap : 2;
		N **c = (N **)realloc(n->callee, newcap * sizeof(N *));
		//logprint("%p = realloc(%p, %d)", c, n->callee, newcap*sizeof(N *));
		if (!c)
			return NULL;
		n->callee = c;
		n->cap = newcap;
	}
	new = newN(f);
	if (!new)
		return NULL;
	new->parent = n;
	++n->len;
	n->callee[n->len - 1] = new;
	return new;
}

/* Increment nsamp in the stack defined by sp. */
static void
sample(N *sp)
{
	for (N *n = sp; n; n = n->parent) {
		pthread_mutex_lock(&n->lsamp);
		++n->nsamp;
		pthread_mutex_unlock(&n->lsamp);
	}
}

/* Grow a buffer. If there is a memory allocation error, jump to nomem. */
static void
growB(B *b)
{
	unsigned newcap = b->cap ? b->cap * 2 : 32;
	char *c = (char *)realloc(b->buf, newcap * sizeof(char));
	if (!c) {
		logprint(FATAL, "growB: realloc: %s", strerror(errno));
		longjmp(nomem, 1);
	}
	b->buf = c;
	b->cap = newcap;
}

/* Append a formatted string to buffer b. Return the number of characters
 * written. It is assumed that the arguments would result
 * in writing at least one character. */
static int
append(B *b, char *fmt, ...)
{
	int wc;
	va_list ap;
retry:
	va_start(ap, fmt);
	wc = vsnprintf(b->buf + b->len, b->cap - b->len, fmt, ap);
	if (wc >= b->cap - b->len) {
		growB(b);
		goto retry;
	}
	va_end(ap);
	b->len += wc;
	return wc;
}

static void
setnotempty(N *n)
{
	n->empty = 0;
	if (n->parent && n->parent->empty)
		setnotempty(n->parent);
}

/* Marshal the given tree n to JSON. The caller is required to first
 * call setjmp(nomem) to catch memory allocation errors. */
static void
marshal(B *b, N *n)
{
	append(b, "{");

	if (n->f.fn)
		append(b, "\"fn\":%ld,", (unsigned long)n->f.fn);

	if (n->f.cs)
		append(b, "\"cs\":%ld,", (unsigned long)n->f.cs);

	pthread_mutex_lock(&n->lcall);
	if (n->ncall)
		append(b, "\"ncalls\":%u,", n->ncall);
	pthread_mutex_unlock(&n->lcall);

	pthread_mutex_lock(&n->lsamp);
	if (n->nsamp)
		append(b, "\"nsamples\":%u,", n->nsamp);
	pthread_mutex_unlock(&n->lsamp);

	/* It's convenient, but hacky, to clear the counters here while we're
	 * walking the tree. All counters are cleared after a tree is emitted,
	 * so that the next tree begins at zero.  We also set the empty flag
	 * so that future calls to marshal can skip empty branches. */
	n->ncall = 0;
	n->nsamp = 0;
	n->empty = 1;

	pthread_mutex_lock(&n->llist);
	if (n->len) {
		append(b, "\"callees\":[");
		for (int i = 0; i < n->len; ++i) {
			if (n->callee[i]->empty)
				continue;
			marshal(b, n->callee[i]);
			append(b, ",");
		}
		if (',' == b->buf[b->len - 1])
			b->len -= 1;
		append(b, "]");
	} else {
		if (',' == b->buf[b->len - 1])
			b->len -= 1;
	}
	pthread_mutex_unlock(&n->llist);

	append(b, "}");
}

/* Return whether the given tree satisfies the property that each parent node's
 * nsamp counter is greater than or equal to the sum of the nsamp counters of
 * its children. */
static int
sane(N *n)
{
	int ok = 1;
	unsigned sum = 0;

	pthread_mutex_lock(&n->lsamp);
	for (int i = 0; i < n->len; ++i) {
		if (!sane(n->callee[i]))
			ok = 0;
		sum += n->callee[i]->nsamp;
	}

	if (n->nsamp < sum) {
		logprint(FATAL, "sane: %p->nsamp = %u, sum = %u", (void *)n, n->nsamp, sum);
		dumpN(FATAL, n, 0);
		ok = 0;
	}
	pthread_mutex_unlock(&n->lsamp);
	return ok;
}

/* Marshal a stacktrace to JSON. The caller is required to first call 
 * setjmp(nomem) to handle memory allocation errors. */
static void
marshals(B *b, N *sp, int sig)
{
	append(b,
	"{"
		"\"signal\":%d,"
		"\"stack_trace\":[", sig);
	for (N *n = sp; n; n = n->parent) {
		append(b,
		"{"
			"\"fn\":%ld,"
			"\"cs\":%ld"
		"},", n->f.fn, n->f.cs);
	}
	if (',' == b->buf[b->len - 1])
		--b->len; /* overwrite trailing comma */
	append(b, "]}");
}

static int
logprint(int level, char *fmt, ...)
{
	B b = {0, 0, 0};
	int ret;
	va_list ap;
	append(&b, "{"
		"\"type\":\"log\","
		"\"data\":{"
			"\"level\":\"%s\","
			"\"message\":\"%s\""
		"}}\n", loglevel[level], fmt);
	va_start(ap, fmt);
	ret = vdprintf(log, b.buf, ap);
	va_end(ap);
	free(b.buf);
	return ret;
}
