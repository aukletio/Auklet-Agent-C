/* Module json implements JSON encoders for profile trees and stack traces. */

/* needs node.h, buf.h */

void marshaltree(Buf *b, Node *n);
void marshalstack(Buf *b, Node *sp, int sig);
