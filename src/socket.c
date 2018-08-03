#include <stdint.h>
#include <pthread.h>

#include "node.h"

#include "socket.h"

#include "buf.h"
#include "json.h"

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

/* connecttoclient connects to the socket provided by an Auklet client and
 * returns 1 if the connection succeeded, othwerise 0. */
int
connecttoclient()
{
	int fd = 4;
	struct stat buf;
	if (-1 == fstat(fd, &buf))
		return 0;
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
			"\"agentVersion\":\"%s\","
			"\"level\":\"%s\","
			"\"message\":\"%s\""
		"}}\n", AUKLET_VERSION, loglevel[level], fmt);
	va_start(ap, fmt);
	ret = vdprintf(fd, b.buf, ap);
	va_end(ap);
	free(b.buf);
	return ret;
}

void
sendstacktrace(int fd, Node *sp, int sig)
{
	Buf b = emptyBuf;
	append(&b, "{"
		"\"type\":\"event\","
		"\"data\":");
	marshalstack(&b, sp, sig);
	append(&b, "}\n");
	write(fd, b.buf, b.len);
	free(b.buf);
}

void
sendprofile(int fd, Node *root)
{
	Buf b = emptyBuf;
	append(&b, "{"
		"\"type\":\"profile\","
		"\"data\":{"
			"\"agentVersion\":\"%s\","
			"\"tree\":", AUKLET_VERSION);
	marshaltree(&b, root);
	append(&b, "}}\n");
	if (-1 != write(fd, b.buf, b.len))
		clearcounters(root);
	free(b.buf);
}
