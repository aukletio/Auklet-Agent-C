#define _XOPEN_SOURCE

#include <stdlib.h>

#include "walloc.h"

#include <fcntl.h>
#include <unistd.h>

static void randseed();

/* global variables */

static int inject = 0;
static double faultrate = 0;

/* exported functions */

/* walloc wraps libc's realloc. */
void *
walloc(void *p, size_t size)
{
	if (inject && drand48() < faultrate)
		return NULL;
	return realloc(p, size);
}

/* injectfaults enables the fault injector and sets the desired fault rate
 * (average number of NULL pointers returned per call to walloc). This is
 * intended for testing. */
void
injectfaults(double rate)
{
	inject = 1;
	faultrate = rate;
	randseed();
}

/* private functions */

/* randseed initializes libc's random number generator with data from the
 * kernel's nonblocking random number source device. */
void
randseed()
{
	long seed;
	int fd = open("/dev/urandom", O_RDONLY, 0);
	read(fd, &seed, sizeof(seed));
	srand48(seed);
}
