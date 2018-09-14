/* we need to test private functions, so we use source inclusion. */
#include "buf.c"

#define len(x) (sizeof(x)/sizeof(x[0]))

void *oom(void *p, size_t size) { return NULL; }

int
test_grow()
{
	struct {
		void *(*walloc)(void *p, size_t size);
		int err;
	} cases[] = {
		{.walloc = realloc, .err = 0},
		{.walloc = oom,     .err = 1},
	};

	int pass = 1;
	for (int i = 0; i < len(cases); i++) {
		walloc = cases[i].walloc;
		int got = grow(&emptyBuf);
		if (got != cases[i].err) {
			pass = 0;
			printf("%s case %d: expected %d, got %d\n", __func__, i, cases[i].err, got);
		}
	}
	walloc = realloc;
	return pass;
}

int
test_append()
{
	struct {
		void *(*walloc)(void *p, size_t size);
		char *arg;
		int err;
	} cases[] = {
		{.walloc = realloc, .arg = "",     .err = 0},
		{.walloc = realloc, .arg = "_",    .err = 0},
		{.walloc = realloc, .arg = "abcd", .err = 0},
		{.walloc = oom,     .arg = "_",    .err = 1},
	};

	int pass = 1;
	for (int i = 0; i < len(cases); i++) {
		walloc = cases[i].walloc;
		int got = append(&emptyBuf, "%s", cases[i].arg);
		if (got != cases[i].err) {
			pass = 0;
			printf("%s case %d: expected %d, got %d\n", __func__, i, cases[i].err, got);
		}
	}
	walloc = realloc;
	return pass;
}

int
main()
{
	int (*tests[])() = {
		test_grow,
		test_append,
	};

	int fails = 0;
	for (int i = 0; i < len(tests); i++)
		if (!tests[i]())
			fails++;

	return fails;
}
