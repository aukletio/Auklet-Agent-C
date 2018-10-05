/* Module json implements JSON encoders for profile trees and stack traces. */

/* needs node.h, buf.h */

void sendstacktrace(Buf *b, int fd, Node *sp, int sig);
void sendprofile(Buf *b, int fd, Node *root);
