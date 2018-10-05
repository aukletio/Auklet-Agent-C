/* Module json implements JSON encoders for profile trees and stack traces. */

/* needs node.h */

void sendstacktrace(int fd, Node *sp, int sig);
void sendprofile(int fd, Node *root);
