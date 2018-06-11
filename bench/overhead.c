#include <time.h>
#include <stdio.h>
#include <stdint.h>

#define MICROSECOND 1000
#define MILLISECOND (1000 * MICROSECOND)
#define SECOND (1000 * MILLISECOND)
#define NOINST __attribute__ ((no_instrument_function))
#define busywait(count) for (int i = 0; i < count; ++i)
#define len(x) (sizeof(x)/sizeof(x[0]))

typedef struct {
	/* count is how many times to call base or inst when calculating
	 * basetime or insttime. */
	int count;

	/* callees is how many distinct callees base or inst should call. */
	int callees;

	/* loops is how many times base or inst should do a busywait loop when
	 * called. If callees > 0, base or inst will not busywait, and loops is
	 * how many times a callee should loop. */
	int loops;

	void (*base)(int, int);
	void (*inst)(int, int);
	int64_t basetime;
	int64_t insttime;
	double overheadpct;
	double overheadabs;
} BM;

NOINST
int64_t
now()
{
	struct timespec t;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
	return (int64_t)SECOND * (int64_t)t.tv_sec + (int64_t)t.tv_nsec;
}

NOINST
int64_t
timefunc(BM *bm, void (*func)(int, int))
{
	int64_t start = now();
	for (int i = 0; i < bm->count; ++i)
		func(bm->loops, bm->callees);
	return now() - start;
}

NOINST
void
run(BM *bm)
{
	if (!bm->count)
		return;
	bm->basetime = timefunc(bm, bm->base);
	bm->insttime = timefunc(bm, bm->inst);
	bm->overheadpct = (double)bm->insttime/(double)bm->basetime;
	bm->overheadabs = (double)(bm->insttime - bm->basetime)/(double)bm->count;
}

char *headfmt = "%8s %8s %11s %10s\n";
char *bmfmt = "%8d %8d %10gx %10g\n";

NOINST
void
printhead()
{
	printf(headfmt, "callees", "loops", "overhead", "ns/call");
}

NOINST
void
print(BM *bm)
{
	if (!bm->count) {
		printf("\n");
		return;
	}
	printf(bmfmt, bm->callees, bm->loops, bm->overheadpct, bm->overheadabs);
}

NOINST
void
base(int loops, int callees)
{
	switch (callees) {
	case 0:
		busywait(loops);
		break;
	case 4: base(loops, 0);
	case 3: base(loops, 0);
	case 2: base(loops, 0);
	case 1: base(loops, 0);
	}
}

void
inst(int loops, int callees)
{
	switch (callees) {
	case 0:
		busywait(loops);
		break;
	case 4: inst(loops, 0);
	case 3: inst(loops, 0);
	case 2: inst(loops, 0);
	case 1: inst(loops, 0);
	}
}

NOINST
int
main()
{
	int count = 4000000;
	BM bm[] = {
		/* count callees loops base  inst */
		{ count, 0,      0,    base, inst },
		{ count, 0,      1<<0, base, inst },
		{ count, 0,      1<<1, base, inst },
		{ count, 0,      1<<2, base, inst },
		{ count, 0,      1<<3, base, inst },
		{ count, 0,      1<<4, base, inst },
		{ count, 0,      1<<5, base, inst },
		{ count, 0,      1<<6, base, inst },
		{ count, 0,      1<<7, base, inst },
		{ 0 },
		{ count, 1,      0,    base, inst },
		{ count, 1,      1<<0, base, inst },
		{ count, 1,      1<<1, base, inst },
		{ count, 1,      1<<2, base, inst },
		{ count, 1,      1<<3, base, inst },
		{ count, 1,      1<<4, base, inst },
		{ count, 1,      1<<5, base, inst },
		{ count, 1,      1<<6, base, inst },
		{ count, 1,      1<<7, base, inst },
		{ 0 },
		{ count/2, 2,      0,    base, inst },
		{ count/2, 2,      1<<0, base, inst },
		{ count/2, 2,      1<<1, base, inst },
		{ count/2, 2,      1<<2, base, inst },
		{ count/2, 2,      1<<3, base, inst },
		{ count/2, 2,      1<<4, base, inst },
		{ count/2, 2,      1<<5, base, inst },
		{ count/2, 2,      1<<6, base, inst },
		{ count/2, 2,      1<<7, base, inst },
		{ 0 },
		{ count/3, 3,      0,    base, inst },
		{ count/3, 3,      1<<0, base, inst },
		{ count/3, 3,      1<<1, base, inst },
		{ count/3, 3,      1<<2, base, inst },
		{ count/3, 3,      1<<3, base, inst },
		{ count/3, 3,      1<<4, base, inst },
		{ count/3, 3,      1<<5, base, inst },
		{ count/3, 3,      1<<6, base, inst },
		{ count/3, 3,      1<<7, base, inst },
		{ 0 },
		{ count/4, 4,      0,    base, inst },
		{ count/4, 4,      1<<0, base, inst },
		{ count/4, 4,      1<<1, base, inst },
		{ count/4, 4,      1<<2, base, inst },
		{ count/4, 4,      1<<3, base, inst },
		{ count/4, 4,      1<<4, base, inst },
		{ count/4, 4,      1<<5, base, inst },
		{ count/4, 4,      1<<6, base, inst },
		{ count/4, 4,      1<<7, base, inst },
	};
	printhead();
	for (int i = 0; i < len(bm); ++i) {
		BM *curr = &bm[i];
		run(curr);
		print(curr);
	}
}
