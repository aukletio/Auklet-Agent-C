#include "node.c"

#include <stdlib.h>
#include <stdio.h>

#include "walloc.h"

#define len(x) (sizeof(x)/sizeof(x[0]))

Frame *emptyFrame = &(Frame){.fn = 0, .cs = 0};

void *oom(void *p, size_t size) { return NULL; }

/* Tests return 1 for success, 0 for failure. */

int
test_newNode()
{
	walloc = oom;
	Node *n = newNode(emptyFrame, NULL);
	walloc = realloc;
	if (n) {
		printf("%s: expected NULL (out of memory), got %p\n", __func__, (void *)n);
		return 0;
	}
	return 1;
}

int
test_freeNode()
{
	freeNode(NULL, 0);
	freeNode(newNode(emptyFrame, NULL), 0);
	return 1;
}

int
test_push()
{
	struct {
		Node *n;
		int expect;
	} cases[] = {
		{.n = NULL,                      .expect = 0},
		{.n = newNode(emptyFrame, NULL), .expect = 1},
	};

	walloc = realloc;
	int pass = 1;
	for (int i = 0; i < len(cases); i++) {
		int got = push(&cases[i].n, emptyFrame);
		if (got != cases[i].expect) {
			printf("%s case %d: expected %d, got %d\n", __func__, i, cases[i].expect, got);
			pass = 0;
		}
	}
	return pass;
}

int
test_pop()
{
	walloc = realloc;
	Node root = emptyNode;
	struct {
		Node *n;
		int expect;
	} cases[] = {
		{.n = &root,                      .expect = 0}, /* no parent */
		{.n = newNode(emptyFrame, &root), .expect = 1}, /* has parent */
	};

	int pass = 1;
	for (int i = 0; i < len(cases); i++) {
		Node *n = cases[i].n;
		int got = pop(&n);
		if (got != cases[i].expect) {
			printf("%s case %d: expected %d, got %d\n", __func__, i, cases[i].expect, got);
			pass = 0;
		}
	}
	return pass;
}

int
test_sample()
{
	Node n = (Node)emptyNode;
	sample(&n);
	return n.nsamp == 1;
}

int
test_equal()
{
	return equal(emptyFrame, emptyFrame);
}

int
test_clearcounters()
{
	Node root = emptyNode;
	Node *sp = &root;
	push(&sp, emptyFrame);
	sample(sp);
	clearcounters(&root);
	return !root.nsamp && !root.ncall;
}

int
main()
{
	int (*tests[])() = {
		test_newNode,
		test_freeNode,
		test_push,
		test_pop,
		test_sample,
		test_equal,
		test_clearcounters,
	};

	int fails = 0;
	for (int i = 0; i < len(tests); i++)
		if (!tests[i]())
			fails++;

	return fails;
}
