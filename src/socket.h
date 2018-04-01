/* Module socket implements an interprocess communication interface to an Auklet client. */

/* needs node.h */

enum {
	DEBUG,
	INFO,
	FATAL,
};

int connecttoclient();
int logprint(int fd, int level, char *fmt, ...);
void sendstacktrace(int fd, Node *sp, int sig);
void sendprofile(int fd, Node *root);
