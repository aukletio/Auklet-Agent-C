#include "socket.h"
#include "buf.h"

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
	int l;
	if ((sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1)
		return 0;
	remote.sun_family = AF_UNIX;
	sprintf(remote.sun_path, "/tmp/auklet-%d", getppid());
	l = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (connect(sockfd, (struct sockaddr *)&remote, l) == -1)
		return 0;
	return 1;
}

int
logprint(int level, char *fmt, ...)
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
	ret = vdprintf(sockfd, b.buf, ap);
	va_end(ap);
	free(b.buf);
	return ret;
}
