/* Module server provides a server to handle profile tree emission requests. */
/* needs pthread.h */

typedef struct {
	int fd;            /* incoming requests */
	void (*handler)(); /* request handler */

	/* internal use */
	pthread_t t;
} Server;

void start(Server *s);
void stop(Server *s);
