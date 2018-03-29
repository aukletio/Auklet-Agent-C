/* Module walloc wraps libc's memory allocation interface. This allows us to
 * inject allocation faults during testing (to simulate OOM situations). */

/* needs stdlib.h */

void *walloc(void *p, size_t size);
void injectfaults(double rate);
