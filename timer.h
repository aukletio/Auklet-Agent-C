/* Module timer provides a timer for controlling the emission of profile trees. */

void starttimer(void (*func)());
void stoptimer();
void siginstall(int sig, void (*handler)(int));
