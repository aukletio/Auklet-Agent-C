/* Module node implements a profile tree data structure. */

/* Each node in the tree represents a callframe. The tree encodes the structure
 * of a program in terms of caller-callee relationships, with each node having a
 * list of callees, and a pointer to the parent, the function that called it.
 * Each node also has counters for number of calls and CPU time (in units of
 * samples). */

/* needs buf.h, stdint.h, pthread.h */

typedef struct {
	uintptr_t fn, cs;
} Frame;

typedef struct Node Node;
struct Node {
	Frame f;
	Node *parent;

	/* lsamp guards nsamp in sample, marshal, and sane. */
	pthread_mutex_t lsamp;
	unsigned nsamp;

	/* llist guards callee, cap, and len in push. */
	pthread_mutex_t llist;
	Node **callee;
	unsigned cap, len;

	pthread_mutex_t lcall;
	unsigned ncall;

	/* empty is used by json.c to mark nodes that can be omitted. */
	int empty;
};

#define emptyNode { \
	.f = {0, 0}, \
	.parent = NULL, \
	.lsamp = PTHREAD_MUTEX_INITIALIZER, \
	.nsamp = 0, \
	.llist = PTHREAD_MUTEX_INITIALIZER, \
	.callee = NULL, \
	.cap = 0, \
	.len = 0, \
	.lcall = PTHREAD_MUTEX_INITIALIZER, \
	.ncall = 0, \
	.empty = 1, \
}

Node *newNode(Frame *f, Node *parent);
void freeNode(Node *n, int root);
int push(Node **sp, Frame *f);
int pop(Node **sp);
void sample(Node *sp);
void clearcounters(Node *n);

void sendstacktrace(
	int fd,
	Node *sp,
	int sig,
	int (*marshal)(Buf *, Node *, int)
);

void sendprofile(
	int fd,
	Node *root,
	int (*marshal)(Buf *, Node *)
);
