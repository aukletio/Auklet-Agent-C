/* Module emit provides a timer for controlling the emission of profile trees. */

void startemitter();
void siginstall(int sig, void (*handler)(int));

/* emitfunc is a callback that is executed when the timer expires. */
void (*emitfunc)();
