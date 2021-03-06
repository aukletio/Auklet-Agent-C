/* Module buf implements a simple, resizable text buffer. */

/* needs stdlib.h */

typedef struct {
	char *buf;
	unsigned cap, len;

	void *(*realloc)(void *, size_t);
	void (*free)(void *);

	int err;
} Buf;

#define emptyBuf(r, f) (Buf){ \
	.buf = NULL, \
	.cap = 0, \
	.len = 0, \
	.realloc = (r), \
	.free = (f), \
	.err = 0, \
}

int append(Buf *b, char *fmt, ...);
