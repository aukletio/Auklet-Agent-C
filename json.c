#include <stdint.h>
#include <pthread.h>

#include "node.h"
#include "buf.h"

#include "json.h"

void 
marshaltree(Buf *b, Node *root)
{
}

void
marshalstack(Buf *b, Node *sp, int sig)
{
	append(b,
	"{"
		"\"signal\":%d,"
		"\"stack_trace\":[", sig);
	for (Node *n = sp; n; n = n->parent) {
		append(b,
		"{"
			"\"fn\":%ld,"
			"\"cs\":%ld"
		"},", n->f.fn, n->f.cs);
	}
	if (',' == b->buf[b->len - 1])
		--b->len; /* overwrite trailing comma */
	append(b, "]}");
}
