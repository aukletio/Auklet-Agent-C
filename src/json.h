/* Module json implements JSON encoders for profile trees and stack traces. */

/* needs node.h, buf.h */

int marshalstacktrace(Buf *b, Node *sp, int sig);
int marshalprofile(Buf *b, Node *root);
