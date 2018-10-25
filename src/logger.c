#include <stdint.h>
#include <pthread.h>

#include "logger.h"

#include "buf.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static char *loglevel[] = {
	[DEBUG] = "debug",
	[INFO] = "info",
	[FATAL] = "fatal",
};

/* exported functions */

int
logprint(int fd, int level, char *fmt, ...)
{
	Buf b = emptyBuf(realloc, free);
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
	b.free(b.buf);
	return ret;
}
