#include <pthread.h>
#include "server.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"

struct Server {
	int fd;            /* incoming requests, outgoing logs */
	void (*handler)(); /* request handler */

	/* internal use */
	pthread_t t;
};

static void *handlerthread(void *p);
static void blocksigs();

/* exported functions */

Server *
newServer(int fd, void (*handler)(), void *(*malloc)(size_t))
{
	Server *s = malloc(sizeof(Server));
	if (!s)
		return NULL;
	*s = (Server){
		.fd = fd,
		.handler = handler,
	};
	return s;
}

int
start(Server *s)
{
	return pthread_create(&s->t, NULL, handlerthread, s);
}

int
wait(Server *s, int kill)
{
	if (kill)
		/* SIGKILL is a valid signal, so we ignore the return value */
		pthread_kill(s->t, SIGKILL);

	return pthread_join(s->t, NULL);
}

/* private functions */

/* blocksigs blocks all signals, preventing the calling thread
 * from being profiled. */
void
blocksigs()
{
	sigset_t s;
	sigfillset(&s);
	pthread_sigmask(SIG_BLOCK, &s, NULL);
}

/* handlerthread handles emission requests by calling s->handler. */
void *
handlerthread(void *p)
{
	Server *s = p;
	char buf;
	size_t rc;

	blocksigs();
	while (1) {
		rc = read(s->fd, &buf, sizeof(buf));
		switch (rc) {
		case 0: /* EOF */
			return NULL;
		case 1: /* ok */
			s->handler();
			break;
		case -1: /* error */
			logprint(s->fd, INFO, "handlerthread: read: %s", strerror(errno));
			break;
		default: /* should be impossible */
			logprint(s->fd, INFO, "handlerthread: read %d bytes", rc);
		}
	}
	return NULL;
}
