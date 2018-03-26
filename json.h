/* Module json implements JSON encoders for profile trees and stack traces. */

/* needs node.h, buf.h */

int marshaltree(Buf *b, Node *n);
int marshalstack(Buf *b, Node *sp, int sig);
