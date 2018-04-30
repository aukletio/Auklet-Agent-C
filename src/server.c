#include "server.h"

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

static void *handlerthread(void *p);

/* global variables */

static void (*reqhandler)() = NULL;
static int sockfd;

/* exported functions */

void
startserver(int fd, void (*handler)())
{
	pthread_t t;
	sockfd = fd;
	reqhandler = handler;
	pthread_create(&t, NULL, handlerthread, NULL);
}

void
stopserver()
{
	reqhandler = NULL;
}

/* private functions */

/* handlerthread handles emission requests by calling reqhandler. */
void *
handlerthread(void *p)
{
	char buf;
	size_t rc;
	sigset_t s;
	sigfillset(&s);
	/* We block all signals to prevent this thread from getting profiled. */
	pthread_sigmask(SIG_BLOCK, &s, NULL);
	while (1) {
		rc = read(sockfd, &buf, sizeof(buf));
		if (rc == -1) {
			/* read failure. */
			continue;
		}
		if (!reqhandler)
			return NULL;
		/* This function can safely use mutexes. */
		reqhandler();
	}
	return NULL;
}
