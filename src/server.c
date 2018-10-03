#include <pthread.h>
#include "server.h"

#include <signal.h>
#include <unistd.h>

static void *handlerthread(void *p);

/* exported functions */

void
start(Server *s)
{
	pthread_create(&s->t, NULL, handlerthread, s);
}

void
stop(Server *s)
{
	s->handler = NULL;
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
	Server *s = (Server *)p;
	char buf;
	size_t rc;

	blocksigs();
	while (1) {
		rc = read(s->fd, &buf, sizeof(buf));
		if (rc == -1) {
			/* read failure. Assume the client has exited. */
			break;
		}
		if (!s->handler)
			return NULL;
		s->handler();
	}
	return NULL;
}
