#include <stdint.h>
#include <pthread.h>

#include "socket.h"

#include "buf.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static char *loglevel[] = {
	[DEBUG] = "debug",
	[INFO] = "info",
	[FATAL] = "fatal",
};

/* exported functions */

/* connecttoclient connects to the socket provided by
 * an Auklet client and returns a valid file descriptor
 * if the connection succeeded, othwerise -1. */
int
connecttoclient()
{
	int fd = 4;
	struct stat buf;

	if (-1 == fstat(fd, &buf))
		return -1;
	return fd;
}

int
logprint(int fd, int level, char *fmt, ...)
{
	Buf b = emptyBuf;
	int ret;
	va_list ap;

	append(&b, "{"
		"\"type\":\"log\","
		"\"data\":{"
			"\"level\":\"%s\","
			"\"message\":\"%s\""
		"}}\n", loglevel[level], fmt);
	va_start(ap, fmt);
	ret = vdprintf(fd, b.buf, ap);
	va_end(ap);
	free(b.buf);
	return ret;
}
