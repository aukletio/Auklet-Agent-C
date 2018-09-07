#include <stdlib.h>
#include "walloc.h"

#include <stdint.h>
#include <pthread.h>

#include "node.h"

#include <stdio.h>

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
	return 1;
}

int
test_push()
{
	walloc = oom;
	struct {
		Node *n;
		int expect;
	} cases[] = {
		{.n = NULL,                      .expect = 0},
		{.n = newNode(emptyFrame, NULL), .expect = 0},
	};

	int pass = 1;
	for (int i = 0; i < len(cases); i++) {
		int got = push(&cases[i].n, emptyFrame);
		if (got != cases[i].expect) {
			printf("%s case %d: expected %d, got %d\n", __func__, i, cases[i].expect, got);
			pass = 0;
		}
	}
	walloc = realloc;
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
main()
{
	int (*tests[])() = {
		test_newNode,
		test_freeNode,
		test_push,
		test_pop,
	};

	int fails = 0;
	for (int i = 0; i < len(tests); i++)
		if (!tests[i]())
			fails++;

	return fails;
}
