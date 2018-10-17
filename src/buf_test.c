/* we need to test private functions, so we use source inclusion. */
#include "buf.c"

#define len(x) (sizeof(x)/sizeof(x[0]))

void *oom(void *p, size_t size) { return NULL; }
void nop(void *p) {}

int
test_grow()
{
	int got;
	int pass = 1;

	struct {
		Buf b;
		int err;
	} c, cases[] = {
		{.b = emptyBuf(realloc, free), .err = 0},
		{.b = emptyBuf(oom, nop),      .err = 1},
	};

	for (int i = 0; i < len(cases); i++) {
		c = cases[i];
		got = grow(&c.b);

		if (got != c.err) {
			pass = 0;
			printf("%s case %d: expected %d, got %d\n", __func__, i, c.err, got);
		}

		c.b.free(c.b.buf);
	}
	return pass;
}

int
test_append()
{
	int got;
	int pass = 1;
	struct {
		Buf b;
		char *arg;
		int err;
	} c, cases[] = {
		{.b = emptyBuf(realloc, free), .arg = "",     .err = 0},
		{.b = emptyBuf(realloc, free), .arg = "_",    .err = 0},
		{.b = emptyBuf(realloc, free), .arg = "abcd", .err = 0},
		{.b = emptyBuf(oom,     nop),  .arg = "_",    .err = 1},
	};

	for (int i = 0; i < len(cases); i++) {
		c = cases[i];
		got = append(&c.b, "%s", c.arg);

		if (got != cases[i].err) {
			pass = 0;
			printf("%s case %d: expected %d, got %d\n", __func__, i, c.err, got);
		}

		c.b.free(c.b.buf);
	}

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
