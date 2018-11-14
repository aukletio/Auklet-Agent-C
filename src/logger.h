/* Module logger provides an interface to send log messages to an Auklet client. */

enum {
	DEBUG,
	INFO,
	FATAL,
};

int logprint(int fd, int level, char *fmt, ...);
