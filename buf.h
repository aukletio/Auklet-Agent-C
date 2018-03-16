typedef struct {
	char *buf;
	unsigned cap, len;
} Buf;

#define emptyBuf (Buf){ \
	.buf = NULL, \
	.cap = 0, \
	.len = 0, \
}

int append(Buf *b, char *fmt, ...);
