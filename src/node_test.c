#include "node.c"

#include <stdlib.h>
#include <stdio.h>

#define len(x) (sizeof(x)/sizeof(x[0]))

Frame *emptyFrame = &(Frame){.fn = 0, .cs = 0};

void *oom(void *p, size_t size) { return NULL; }

/* Tests return 1 for success, 0 for failure. */

int
test_newNode()
{
	Node root = emptyNode(oom);
	Node *n = newNode(emptyFrame, &root);
	if (n) {
		printf("%s: expected NULL (out of memory), got %p\n", __func__, (void *)n);
		return 0;
	}
	return 1;
}

int
test_freeNode()
{
	Node root = emptyNode(realloc);
	Node *sp = &root;
	push(&sp, emptyFrame);
	freeNode(&root, 1, free);
	return 1;
}

int
test_push()
{
	Node root = emptyNode(realloc);
	int got, pass;
	struct {
		Node *sp;
		int expect;
	} c, cases[] = {
		{.sp = &root, .expect = 1},
	};

	pass = 1;
	for (int i = 0; i < len(cases); i++) {
		c = cases[i];
		got = push(&c.sp, emptyFrame);
		if (got != c.expect) {
			printf("%s case %d: expected %d, got %d\n", __func__, i, c.expect, got);
			pass = 0;
		} else {
			freeNode(&root, 1, free);
		}
	}
	return pass;
}

int
test_pop()
{
	int got, pass;
	Node root = emptyNode(realloc);
	struct {
		Node *n;
		int expect;
	} c, cases[] = {
		{.n = &root,                      .expect = 0}, /* no parent */
		{.n = newNode(emptyFrame, &root), .expect = 1}, /* has parent */
	};

	pass = 1;
	for (int i = 0; i < len(cases); i++) {
		c = cases[i];
		got = pop(&c.n);
		if (got != c.expect) {
			printf("%s case %d: expected %d, got %d\n", __func__, i, c.expect, got);
			pass = 0;
		}
	}
	return pass;
}

int
test_sample()
{
	Node root = emptyNode(NULL);
	sample(&root);
	return root.nsamp == 1;
}

int
test_equal()
{
	return equal(emptyFrame, emptyFrame);
}

int
test_clearcounters()
{
	Node root = emptyNode(realloc);
	Node *sp = &root;

	push(&sp, emptyFrame);
	sample(sp);
	clearcounters(&root);

	freeNode(&root, 1, free);
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
