/* Module timer provides a timer for controlling the emission of profile trees. */

/* needs unistd.h */

void starttimer(void (*func)());
void siginstall(int sig, void (*handler)(int));
