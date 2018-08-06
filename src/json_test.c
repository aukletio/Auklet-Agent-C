#include <stdint.h>
#include <pthread.h>
#include "node.h"
#include "buf.h"
#include "json.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define len(x) (sizeof(x)/sizeof(x[0]))

typedef struct {
	const char *name;
	Node *node;
	char *want;
	Buf got;
} Case;

void
marshaltree_case0(Case *c)
{
	*c = (Case){
		.name = __func__,
		.node = newNode(&(Frame){0, 0}, NULL),
		.want = NULL,
	};
	marshaltree(&c->got, c->node);
}

void
marshaltree_case1(Case *c)
{
	*c = (Case){
		.name = __func__,
		.node = newNode(&(Frame){0, 0}, NULL),
		.want = "{"
			"\"nSamples\":1,"
			"\"callees\":[]"
		"}",
	};
	sample(c->node);
	marshaltree(&c->got, c->node);
}

void
marshaltree_case2(Case *c)
{
	*c = (Case){
		.name = __func__,
		.node = newNode(&(Frame){0, 0}, NULL),
		.want = "{"
			"\"callees\":["
				"{"
					"\"nCalls\":1,"
					"\"callees\":[]"
				"}"
			"]"
		"}",
	};
	Node *sp = c->node;
	push(&sp, &(Frame){0, 0});
	marshaltree(&c->got, c->node);
}

void
marshalstack_case0(Case *c)
{
	*c = (Case){
		.name = __func__,
		.node = newNode(&(Frame){0, 0}, NULL),
		.want = "{"
			"\"signal\":\"Unknown signal 0\","
			"\"stackTrace\":["
				"{"
					"\"functionAddress\":0,"
					"\"callSiteAddress\":0"
				"}"
			"]"
		"}",
	};
	marshalstack(&c->got, c->node, 0);
}

int
compare(char *a, char *b)
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
main()
{
	void (*test[])(Case *) = {
		marshaltree_case0,
		marshaltree_case1,
		marshaltree_case2,
		marshalstack_case0,
	};
	int ret = 0;
	Case k;
	for (int i = 0; i < len(test); ++i) {
		k.got = emptyBuf;
		test[i](&k);
		int fail = compare(k.want, k.got.buf);
		printf("%d/%lu %s %s\n", i+1, len(test), fail ? "fail" : "pass", k.name);
		if (fail) {
			ret = 1;
			printf("wanted %s\n"
			       "got    %s\n", k.want, k.got.buf);
		}
		freeNode(k.node, 0);
		free(k.got.buf);
	}
	return ret;
}
