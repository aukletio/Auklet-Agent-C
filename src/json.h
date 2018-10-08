/* Module json implements JSON encoders for profile trees and stack traces. */

/* needs node.h, buf.h */

int sendstacktrace(Buf *b, int fd, Node *sp, int sig);
int sendprofile(Buf *b, int fd, Node *root);
