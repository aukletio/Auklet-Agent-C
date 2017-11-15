/*
 * unit tester for profiler library
 */

#include "lib.c"

#define str(x) #x

F z = {0, 0};
F f = {0xaced, 0xfade};

int
callee_test(void)
{
	N *root = newN(z);
	N *c = addcallee(root, f);
	//dumpN(root, 0);
	N *g = hascallee(root, f);
	int ret = 1;
	if (c != g) {
		printf("callee_test: expected %p, got %p\n", (void *)c, (void *)g);
		ret = 0;
	}
	killN(root, 0);
	return ret;
}

int
marshal_test(void)
{
	N *root = newN(z);
	B b = {0, 0, 0};
	char *e = "{}";
	int ret = 1;
	if (!marshal(&b, root)) {
		printf("marshal_test: marshal failed\n");
		ret = 0;
	}
	if (strcmp(b.buf, e)) {
		printf("marshal_test: expected \"%s\", got \"%s\"\n", e, b.buf);
		ret = 0;
	}
	killN(root, 0);
	free(b.buf);
	return ret;
}

int
marshal_test2(void)
{
	N *root = newN(z);
	B b = {0, 0, 0};
	char *e = "{\"callees\":[{"
		"\"fn\":44269,"
		"\"cs\":64222"
	"}]}";
	int ret = 1;
	addcallee(root, f);
	if (!marshal(&b, root)) {
		printf("marshal_test2: marshal failed\n");
		ret = 0;
	}
	if (strcmp(b.buf, e)) {
		printf("marshal_test2: expected \"%s\"\n"
		       "               got      \"%s\"\n", e, b.buf);
		ret = 0;
	}
	killN(root, 0);
	free(b.buf);
	return ret;
}

int
main()
{
	struct {
		int (*run)(void);
		char *name;
	} test[] = {
#define TEST(f) {f, str(f)}
		TEST(callee_test),
		TEST(marshal_test),
		TEST(marshal_test2),
	};
	int ret = 0;
	for (int i = 0; i < len(test); ++i) {
		int pass = test[i].run();
		printf("%d/%lu %15s: %s\n", i+1, len(test),
		       test[i].name,
		       pass ? "pass" : "fail");
		if (!pass)
			ret = 1;
	}
	return ret;
}
