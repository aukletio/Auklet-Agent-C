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
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static char *loglevel[] = {
	[DEBUG] = "debug", // memory alloc
	[INFO] = "info",   // init message
	[FATAL] = "fatal", // unrecoverable error
};

/* exported functions */

/* connecttoclient connects to the socket provided by an Auklet client and
 * returns 1 if the connection succeeded, othwerise 0. */
int
connecttoclient()
{
	struct sockaddr_un remote;
	int l, fd;
	if ((fd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1)
		return 0;
	remote.sun_family = AF_UNIX;
	sprintf(remote.sun_path, "/tmp/auklet-%d", getppid());
	l = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (connect(fd, (struct sockaddr *)&remote, l) == -1)
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
			"\"level\":\"%s\","
			"\"message\":\"%s\""
		"}}\n", loglevel[level], fmt);
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
	append(&b, "{\"type\":\"event\",\"data\":");
	marshalstack(&b, sp, sig);
	append(&b, "}\n");
	write(fd, b.buf, b.len);
	free(b.buf);
}

void
sendprofile(int fd, Node *root)
{
	Buf b = emptyBuf;
	append(&b, "{\"type\":\"profile\",\"data\":{\"tree\":");
	marshaltree(&b, root);
	append(&b, "}}\n");
	if (-1 != write(fd, b.buf, b.len))
		clearcounters(root);
	free(b.buf);
}
