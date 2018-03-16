#include "buf.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static int grow(Buf *b);

/* exported functions */

/* append appends a formatted string to b and returns the number of characters
 * appended. If an error occurs, it returns -1. */
int
append(Buf *b, char *fmt, ...)
{
	int wc;
	va_list ap;
retry:
	va_start(ap, fmt);
	wc = vsnprintf(b->buf + b->len, b->cap - b->len, fmt, ap);
	if (wc >= b->cap - b->len) {
		if (grow(b))
			return -1;
		goto retry;
	}
	va_end(ap);
	b->len += wc;
	return wc;
}

/* private functions */

/* grow increases b's capacity. If there is a memory allocation error, it
 * returns -1. */
int
grow(Buf *b)
{
	unsigned newcap = b->cap ? b->cap * 2 : 32;
	char *c = realloc(b->buf, newcap * sizeof(char));
	if (!c) {
		return -1;
	}
	b->buf = c;
	b->cap = newcap;
	return 0;
}
