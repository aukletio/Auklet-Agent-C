/*
 * runtime test program
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BILLION 1000000000l

long
now(void)
{
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	return t.tv_sec * BILLION + t.tv_nsec;
}

int
f(int x)
{
	if (x < 2)
		return x;
	else
		return f(x - 1) + f(x - 2);
}

void *
busy(void *p)
{
	int n = *(int *)p;
	long end = now() + 4 * BILLION;
	int i;
	for (i = 0; now() < end; ++i)
		f(n);
	printf("busy: did %d iterations of f(%d)\n", i, n);
}

void *
idle(void *p)
{
	int rem = 24;
	while (rem)
		rem = sleep(rem);
	int *n;
	*n = 42;
}

int
main(int argc, char **argv)
{
	int n = 0, m = 0;
	pthread_t a, b;
	if (argc > 1)
		n = atoi(argv[1]);
	m = n + 1;
	pthread_create(&a, NULL, idle, &n);
	busy(&m);
	pthread_join(a, NULL);
	return 0;
}
