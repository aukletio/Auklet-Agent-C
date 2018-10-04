#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "buf.h"
#include "node.h"

#include "json.h"

#define len(x) (sizeof(x)/sizeof(x[0]))

static int isroot(Node *n);
static int marshalNode(Buf *b, Node *n);
static int appendLeftBrace(Buf *b, Node *n);
static int appendRightBrace(Buf *b, Node *n);
static int appendFn(Buf *b, Node *n);
static int appendCs(Buf *b, Node *n);
static int appendCalls(Buf *b, Node *n);
static int appendSamples(Buf *b, Node *n);
static int appendCalleesKey(Buf *b, Node *n);
static int appendCalleesValue(Buf *b, Node *n);
static int appendRightBracket(Buf *b, Node *n);
static int markempty(Node *n);
static int markemptycallees(Node *n);

/* exported functions */

int
marshaltree(Buf *b, Node *root)
{
	markempty(root);
	return marshalNode(b, root);
}

int
marshalstack(Buf *b, Node *sp, int sig)
{
	append(b,
	"{"
		"\"signal\":\"%s\","
		"\"stackTrace\":[", strsignal(sig));
	for (Node *n = sp; n; n = n->parent) {
		if (n != sp) {
			/* we've passed the first node */
			append(b, ",");
		}

		append(b,
		"{"
			"\"functionAddress\":%ld,"
			"\"callSiteAddress\":%ld"
		"}", n->f.fn, n->f.cs);
	}
	append(b, "]}");
	return 0;
}

/* private functions */

int
isroot(Node *n)
{
	return n && !n->f.fn && !n->f.cs;
}

int
marshalNode(Buf *b, Node *n)
{
	int (*action[])(Buf *, Node *) = {
		appendLeftBrace,
		appendFn,
		appendCs,
		appendCalls,
		appendSamples,
		appendCalleesKey,
		appendCalleesValue,
		appendRightBracket,
		appendRightBrace,
	};

	if (!n || n->empty)
		return 0;

	for (int i = 0; i < len(action); i++) {
		int err = action[i](b, n);
		if (err)
			return err;
	}
	return 0;
}

int appendLeftBrace(Buf *b, Node *n) { return append(b, "{"); }
int appendRightBrace(Buf *b, Node *n) { return append(b, "}"); }

int
appendFn(Buf *b, Node *n)
{
	if (n->f.fn)
		return append(b, "\"functionAddress\":%ld,", (unsigned long)n->f.fn);
	return 0;
}

int
appendCs(Buf *b, Node *n)
{
	if (n->f.cs && !isroot(n->parent)) {
		/* We don't want to marshal the callsite if the parent is root.
		 * This is because any child of root does not have a meaningful
		 * callsite. */
		return append(b, "\"callSiteAddress\":%ld,", (unsigned long)n->f.cs);
	}
	return 0;
}

int
appendCalls(Buf *b, Node *n)
{
	int err = 0;
	pthread_mutex_lock(&n->lcall);
	if (n->ncall)
		err = append(b, "\"nCalls\":%u,", n->ncall);
	pthread_mutex_unlock(&n->lcall);
	return err;
}

int
appendSamples(Buf *b, Node *n)
{
	int err = 0;
	pthread_mutex_lock(&n->lsamp);
	if (n->nsamp)
		err = append(b, "\"nSamples\":%u,", n->nsamp);
	pthread_mutex_unlock(&n->lsamp);
	return err;
}

int
appendCalleesKey(Buf *b, Node *n)
{
	return append(b, "\"callees\":[");
}

int
appendCalleesValue(Buf *b, Node *n)
{
	int err = 0;
	pthread_mutex_lock(&n->llist);
	for (int i = 0; i < n->len; ++i) {
		if (i) {
			err = append(b, ",");
			if (err)
				goto end;
		}

		err = marshalNode(b, n->callee[i]);
		if (err)
			goto end;
	}
end:
	pthread_mutex_unlock(&n->llist);
	return err;
}

int appendRightBracket(Buf *b, Node *n) { return append(b, "]"); }

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
