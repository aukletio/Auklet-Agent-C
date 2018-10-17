#include <stdlib.h>

#include "buf.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>

static int grow(Buf *b);

/* exported functions */

/* append appends a formatted string to b. If an error occurs, 
 * it returns 1; otherwise, 0. */
int
append(Buf *b, char *fmt, ...)
{
	int wc, err;
	va_list ap;

	if (b->err)
		return b->err;

retry:
	va_start(ap, fmt);
	wc = vsnprintf(b->buf + b->len, b->cap - b->len, fmt, ap);
	if (wc >= b->cap - b->len) {
		err = grow(b);
		if (err) {
			b->err = 1;
			return 1;
		}
		goto retry;
	}
	va_end(ap);
	b->len += wc;

	return 0;
}

/* private functions */

/* grow increases b's capacity. If there is a memory allocation error, it
 * returns 1; otherwise, 0. */
int
grow(Buf *b)
{
	unsigned newcap;
	char *c;

	if (b->err)
		return b->err;

	newcap = b->cap ? b->cap * 2 : 32;
	c = b->realloc(b->buf, newcap * sizeof(char));
	if (!c) {
		b->err = 1;
		return 1;
	}
	b->buf = c;
	b->cap = newcap;
	return 0;
}
