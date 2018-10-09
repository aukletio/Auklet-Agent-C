#include "json.c"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

Frame *emptyFrame = &(Frame){.fn = 0, .cs = 0};

int differ(char *a, char *b);

void nop(Node **sp) {}
void sampled(Node **sp) { sample(*sp); }
void pushed(Node **sp) { push(sp, &(Frame){3, 4}); }

void
twocallees(Node **sp)
{
	push(sp, &(Frame){5, 6});
	pop(sp);
	push(sp, &(Frame){7, 8});
}

void
twopushedcallees(Node **sp)
{
	push(sp, &(Frame){3, 4});
	push(sp, &(Frame){5, 6});
	pop(sp);
	push(sp, &(Frame){7, 8});
}

int
test_marshaltree()
{
	Node root, *sp;
	int pass = 1;
	int err;
	Buf got;

	struct {
		void (*stackop)(Node **);
		char *want;
	} c, cases[] = {
		{
			.stackop = nop,
			.want = NULL,
		},
		{
			.stackop = sampled,
			.want = "{"
				"\"nSamples\":1,"
				"\"callees\":[]"
			"}",
		},
		{
			.stackop = pushed,
			.want = "{"
				"\"callees\":["
					"{"
						"\"functionAddress\":3,"
						"\"nCalls\":1,"
						"\"callees\":[]"
					"}"
				"]"
			"}",
		},
		{
			.stackop = twocallees,
			.want = "{"
				"\"callees\":["
					"{"
						"\"functionAddress\":5,"
						"\"nCalls\":1,"
						"\"callees\":[]"
					"},"
					"{"
						"\"functionAddress\":7,"
						"\"nCalls\":1,"
						"\"callees\":[]"
					"}"
				"]"
			"}",
		},
		{
			.stackop = twopushedcallees,
			.want = "{"
				"\"callees\":["
					"{"
						"\"functionAddress\":3,"
						"\"nCalls\":1,"
						"\"callees\":["
							"{"
								"\"functionAddress\":5,"
								"\"callSiteAddress\":6,"
								"\"nCalls\":1,"
								"\"callees\":[]"
							"},"
							"{"
								"\"functionAddress\":7,"
								"\"callSiteAddress\":8,"
								"\"nCalls\":1,"
								"\"callees\":[]"
							"}"
						"]"
					"}"
				"]"
			"}",
		},
	};

	for (int i = 0; i < len(cases); i++) {
		c = cases[i];
		got = emptyBuf(realloc, free);

		root = (Node)emptyNode(realloc);
		sp = &root;
		c.stackop(&sp);

		err = marshaltree(&got, &root);
		if (err) {
			pass = 0;
			printf("%s case %d: error: %d\n", __func__, i, err);
		}

		if (differ(c.want, got.buf)) {
			pass = 0;
			printf("%s case %d:\n"
			       "  wanted %s\n"
			       "  got    %s\n", __func__, i, c.want, got.buf);
		}

		freeNode(&root, 1, free);
		got.free(got.buf);
	}

	return pass;
}

int
test_marshalstack()
{
	Node root = emptyNode(realloc);
	Node *sp;
	int pass = 1;
	int err;
	Buf got;

	sp = &root;
	push(&sp, &(Frame){3, 4});

	struct {
		Node *node;
		char *want;
	} c, cases[] = {
		{
			.node = &(Node)emptyNode(realloc),
			.want = "{"
				"\"signal\":\"Unknown signal 0\","
				"\"stackTrace\":["
					"{"
						"\"functionAddress\":0,"
						"\"callSiteAddress\":0"
					"}"
				"]"
			"}",
		},
		{
			.node = sp,
			.want = "{"
				"\"signal\":\"Unknown signal 0\","
				"\"stackTrace\":["
					"{"
						"\"functionAddress\":3,"
						"\"callSiteAddress\":4"
					"},"
					"{"
						"\"functionAddress\":0,"
						"\"callSiteAddress\":0"
					"}"
				"]"
			"}",
		},
	};

	for (int i = 0; i < len(cases); i++) {
		c = cases[i];
		got = emptyBuf(realloc, free);

		err = marshalstack(&got, c.node, 0);
		if (err) {
			pass = 0;
			printf("%s case %d: error: %d\n", __func__, i, err);
		}

		if (differ(c.want, got.buf)) {
			pass = 0;
			printf("%s case %d:\n"
			       "  wanted %s\n"
			       "  got    %s\n", __func__, i, c.want, got.buf);
		}

		freeNode(c.node, 1, free);
		got.free(got.buf);
	}

	return pass;
}

int
differ(char *a, char *b)
{
	if (!a && !b) {
		/* two null strings are equal. */
		return 0;
	} else if (!a || !b) {
		/* one null string is not equal to another non-null string. */
		return 1;
	}
	return strcmp(a, b);
}

int
test_marshal()
{
	Buf b;
	int pass;
	struct {
		Node *n;
	} c, cases[] = {
		{
			.n = &(Node)emptyNode(realloc),
		},
	};

	pass = 1;
	for (int i = 0; i < len(cases); i++) {
		c = cases[i];

		b = emptyBuf(realloc, free);
		marshalstacktrace(&b, c.n, 0);
		if (b.err) {
			pass = 0;
			printf("%s case %d: marshalstacktrace: %s\n", __func__, i, strerror(errno));
		}
		b.free(b.buf);

		b = emptyBuf(realloc, free);
		marshalprofile(&b, c.n);
		if (b.err) {
			pass = 0;
			printf("%s case %d: marshalprofile: %s\n", __func__, i, strerror(errno));
		}
		b.free(b.buf);
	}

	return pass;
}

int
main()
{
	int fails = 0;
	int (*tests[])() = {
		test_marshaltree,
		test_marshalstack,
		test_marshal,
	};

	for (int i = 0; i< len(tests); i++)
		if (!tests[i]())
			fails++;

	return fails;
}
