#include <stdint.h>
#include <pthread.h>

#include "node.h"

#include <stdlib.h>

static Node *newNode(Frame *f, Node *parent);
static int equal(Frame *a, Frame *b);
static Node *get(Node *n, Frame *f);
static Node *add(Node *n, Frame *f);
static Node *getoradd(Node *n, Frame *f);
static int grow(Node *n);

/* exported functions */

void
freeNode(Node *n, int root, void (*free)(void *))
{
	if (!n)
		return;
	for (int i = 0; i < n->len; ++i)
		freeNode(n->callee[i], 0, free);
	free(n->callee);
	if (!root)
		free(n);
}

int
push(Node **sp, Frame *f)
{
	Node *c = getoradd(*sp, f);
	if (!c)
		return 0;
	pthread_mutex_lock(&c->lcall);
	++c->ncall;
	pthread_mutex_unlock(&c->lcall);
	*sp = c;
	return 1;
}

int
pop(Node **sp)
{
	Node *n = *sp;
	if (!n->parent)
		return 0;
	*sp = n->parent;
	return 1;
}

void
sample(Node *sp)
{
	for (Node *n = sp; n; n = n->parent) {
		pthread_mutex_lock(&n->lsamp);
		++n->nsamp;
		pthread_mutex_unlock(&n->lsamp);
	}
}

/* private functions */

Node *
newNode(Frame *f, Node *parent)
{
	Node *n = parent->realloc(NULL, sizeof(Node));
	if (!n)
		return NULL;
	*n = (Node)emptyNode(parent->realloc);
	n->f = *f;
	n->parent = parent;
	return n;
}

int
equal(Frame *a, Frame *b)
{
	return ((a->fn == b->fn) && (a->cs == b->cs));
}

Node *
get(Node *n, Frame *f)
{
	for (int i = 0; i < n->len; ++i)
		if (equal(&n->callee[i]->f, f))
			return n->callee[i];
	return NULL;
}

Node *
add(Node *n, Frame *f)
{
	if (grow(n))
		return NULL;
	Node *new = newNode(f, n);
	if (!new)
		return NULL;
	++n->len;
	n->callee[n->len - 1] = new;
	return new;
}

Node *
getoradd(Node *n, Frame *f)
{
	pthread_mutex_lock(&n->llist);
	Node *c = get(n, f);
	if (!c)
		c = add(n, f);
	pthread_mutex_unlock(&n->llist);
	return c;
}

int
grow(Node *n)
{
	if (n->len < n->cap)
		return 0;
	unsigned cap = n->cap ? 2 * n->cap : 1;
	Node **callee = n->realloc(n->callee, cap*sizeof(Node *));
	if (!callee)
		return -1;
	n->callee = callee;
	n->cap = cap;
	return 0;
}

void
clearcounters(Node *n)
{
	n->nsamp = 0;
	n->ncall = 0;
	for (int i = 0; i < n->len; ++i)
		clearcounters(n->callee[i]);
}
