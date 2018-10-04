/* Module socket implements an interprocess communication interface to an Auklet client. */

enum {
	DEBUG,
	INFO,
	FATAL,
};

int connecttoclient();
int logprint(int fd, int level, char *fmt, ...);
