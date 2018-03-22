#include <stdint.h>
#include <pthread.h>

#include "node.h"
#include "buf.h"

#include "json.h"

static void marshalNode(Buf *b, Node *n);
static void removetrailingcomma(Buf *b);
static int markempty(Node *n);
static int markemptycallees(Node *n);

/* exported functions */

void 
marshaltree(Buf *b, Node *root)
{
	markempty(root);
	marshalNode(b, root);
	removetrailingcomma(b);
}

void
marshalstack(Buf *b, Node *sp, int sig)
{
	append(b,
	"{"
		"\"signal\":%d,"
		"\"stack_trace\":[", sig);
	for (Node *n = sp; n; n = n->parent) {
		append(b,
		"{"
			"\"fn\":%ld,"
			"\"cs\":%ld"
		"},", n->f.fn, n->f.cs);
	}
	removetrailingcomma(b);
	append(b, "]}");
}

/* private functions */

/* marshalNode marshals the given tree n to JSON. The caller is required to
 * first call setjmp(nomem) to catch memory allocation errors. */
void
marshalNode(Buf *b, Node *n)
{
	if (n->empty)
		return;

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

	pthread_mutex_lock(&n->llist);
	append(b, "\"callees\":[");
	for (int i = 0; i < n->len; ++i)
		marshalNode(b, n->callee[i]);

	removetrailingcomma(b);

	append(b, "]");
	pthread_mutex_unlock(&n->llist);
	append(b, "},");
}

void
removetrailingcomma(Buf *b)
{
	if (!b->buf)
		return;
	if (',' == b->buf[b->len - 1]) {
		b->len -= 1;
		b->buf[b->len] = '\0';
	}
}

/* markempty determines if n is empty and sets n->empty accordingly. It also
 * returns this value. */
int
markempty(Node *n)
{
	/* markemptycallees must be called first because it must be run
	 * unconditionally. Otherwise short-circuiting will terminate the
	 * statement early and the callees won't get marked. */
	n->empty = markemptycallees(n) && !n->ncall && !n->nsamp;
	return n->empty;
}

/* markemptycallees marks each empty callee of n as empty. If at least one
 * callee is empty, it returns 0; otherwise 1. */
int
markemptycallees(Node *n)
{
	/* Assume empty, initially. This ensures that if n->len == 0, we count
	 * that as n's callees being empty. */
	int empty = 1;
	for (int i = 0; i < n->len; ++i)
		if (!markempty(n->callee[i]))
			empty = 0;
	return empty;
}
