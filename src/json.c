#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "buf.h"
#include "node.h"

#include "json.h"

#include <stdlib.h>
#include <unistd.h>

#define len(x) (sizeof(x)/sizeof(x[0]))

static int marshaltree(Buf *b, Node *n);
static int marshalstack(Buf *b, Node *sp, int sig);
static int isroot(Node *n);
static int marshalNode(Buf *b, Node *n);
static int markempty(Node *n);
static int markemptycallees(Node *n);

/* exported functions */

int
marshalstacktrace(Buf *b, Node *sp, int sig)
{
	if (b->err)
		return b->err;

	append(b, "{\"type\":\"event\",\"data\":");
	marshalstack(b, sp, sig);
	append(b, "}\n");
	return b->err;
}

int
marshalprofile(Buf *b, Node *root)
{
	if (b->err)
		return b->err;

	append(b, "{\"type\":\"profile\",\"data\":{\"tree\":");
	marshaltree(b, root);
	append(b, "}}\n");
	return b->err;
}

/* private functions */

int
marshaltree(Buf *b, Node *root)
{
	markempty(root);
	return marshalNode(b, root);
}

int
marshalstack(Buf *b, Node *sp, int sig)
{
	if (b->err)
		return b->err;

	append(b, "{"
		"\"signal\":\"%s\","
		"\"stackTrace\":[", strsignal(sig));

	for (Node *n = sp; n; n = n->parent) {
		if (n != sp)
			/* we've passed the first node */
			append(b, ",");

		append(b, "{"
			"\"functionAddress\":%ld,"
			"\"callSiteAddress\":%ld"
		"}", n->f.fn, n->f.cs);
	}
	append(b, "]}");
	return b->err;
}

int
isroot(Node *n)
{
	return n && !n->f.fn && !n->f.cs;
}

int
marshalNode(Buf *b, Node *n)
{
	if (b->err)
		return b->err;

	append(b, "{");

	if (n->f.fn)
		append(b, "\"functionAddress\":%ld,", (unsigned long)n->f.fn);

	if (n->f.cs && !isroot(n->parent)) {
		/* We don't want to marshal the callsite if the parent is root.
		 * This is because any child of root does not have a meaningful
		 * callsite. */
		append(b, "\"callSiteAddress\":%ld,", (unsigned long)n->f.cs);
	}

	if (n->empty)
		goto end;

	pthread_mutex_lock(&n->lcall);
	if (n->ncall)
		append(b, "\"nCalls\":%u,", n->ncall);
	pthread_mutex_unlock(&n->lcall);

	pthread_mutex_lock(&n->lsamp);
	if (n->nsamp)
		append(b, "\"nSamples\":%u,", n->nsamp);
	pthread_mutex_unlock(&n->lsamp);

	append(b, "\"callees\":[");

	pthread_mutex_lock(&n->llist);
	for (int i = 0; i < n->len; ++i) {
		if (i)
			append(b, ",");

		marshalNode(b, n->callee[i]);
	}
	pthread_mutex_unlock(&n->llist);

	append(b, "]");
end:
	append(b, "}");
	return b->err;
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
	n->empty = markemptycallees(n);

	pthread_mutex_lock(&n->lcall);
	n->empty = n->empty && !n->ncall;
	pthread_mutex_unlock(&n->lcall);

	pthread_mutex_lock(&n->lsamp);
	n->empty = n->empty && !n->nsamp;
	pthread_mutex_unlock(&n->lsamp);
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
	pthread_mutex_lock(&n->llist);
	for (int i = 0; i < n->len; ++i) // <-- race
		if (!markempty(n->callee[i]))
			empty = 0;
	pthread_mutex_unlock(&n->llist);
	return empty;
}
