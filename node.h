/* needs stdint.h, pthread.h */

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

	int empty;
};

#define emptyNode (Node){ \
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
Node *push(Node *sp, Frame *f);
Node *pop(Node *sp);
void sample(Node *sp);
