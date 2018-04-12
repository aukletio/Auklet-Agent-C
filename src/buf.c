#include "buf.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void grow(Buf *b);

/* global variables */

/* nomem is a jump point for recovering from memory allocation errors. */
static jmp_buf nomem;

/* exported functions */

/* append appends a formatted string to b and returns the number of characters
 * appended. If an error occurs, execution jumps to bufcatch. */
int
append(Buf *b, char *fmt, ...)
{
	int wc;
	va_list ap;
retry:
	va_start(ap, fmt);
	wc = vsnprintf(b->buf + b->len, b->cap - b->len, fmt, ap);
	if (wc >= b->cap - b->len) {
		grow(b);
		goto retry;
	}
	va_end(ap);
	b->len += wc;
	return wc;
}

/* bufcatch sets a jump point for recovering from memory allocation errors. It
 * is only useful on systems that have disabled overcommit. */
int
bufcatch()
{
	return setjmp(nomem);
}

/* private functions */

/* grow increases b's capacity. If there is a memory allocation error, it
 * jumps to nomem. */
void
grow(Buf *b)
{
	unsigned newcap = b->cap ? b->cap * 2 : 32;
	char *c = realloc(b->buf, newcap * sizeof(char));
	if (!c)
		longjmp(nomem, 1);
	b->buf = c;
	b->cap = newcap;
}
