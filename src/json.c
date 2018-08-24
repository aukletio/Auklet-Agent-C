#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "node.h"
#include "buf.h"

#include "json.h"

static int isroot(Node *n);
static void marshalNode(Buf *b, Node *n);
static void removetrailingcomma(Buf *b);
static int markempty(Node *n);
static int markemptycallees(Node *n);

/* exported functions */

int
marshaltree(Buf *b, Node *root)
{
	int err = 0;
	err = bufcatch();
	if (err)
		return -1;
	markempty(root);
	marshalNode(b, root);
	removetrailingcomma(b);
	return 0;
}

int
marshalstack(Buf *b, Node *sp, int sig)
{
	int err = 0;
	err = bufcatch();
	if (err)
		return -1;
	append(b,
	"{"
		"\"signal\":\"%s\","
		"\"stackTrace\":[", strsignal(sig));
	for (Node *n = sp; n; n = n->parent) {
		append(b,
		"{"
			"\"functionAddress\":%ld,"
			"\"callSiteAddress\":%ld"
		"},", n->f.fn, n->f.cs);
	}
	removetrailingcomma(b);
	append(b, "]}");
	return 0;
}

/* private functions */

int
isroot(Node *n)
{
	return n && !n->f.fn && !n->f.cs;
}

/* marshalNode marshals the given tree n to JSON. The caller is required to
 * first call bufcatch to catch memory allocation errors. */
void
marshalNode(Buf *b, Node *n)
{
	if (!n || n->empty)
		return;

	append(b, "{");
	if (n->f.fn)
		append(b, "\"functionAddress\":%ld,", (unsigned long)n->f.fn);
	if (n->f.cs && !isroot(n->parent)) {
		/* We don't want to marshal the callsite if the parent is root.
		 * This is because any child of root does not have a meaningful
		 * callsite. */
		append(b, "\"callSiteAddress\":%ld,", (unsigned long)n->f.cs);
	}

	pthread_mutex_lock(&n->lcall);
	if (n->ncall)
		append(b, "\"nCalls\":%u,", n->ncall);
	pthread_mutex_unlock(&n->lcall);

	pthread_mutex_lock(&n->lsamp);
	if (n->nsamp)
		append(b, "\"nSamples\":%u,", n->nsamp);
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
	if (!n)
		return 0;
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
