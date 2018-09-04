/* Module walloc wraps libc's memory allocation interface. This allows us to
 * inject allocation faults during testing (to simulate OOM situations). */

/* needs stdlib.h */

/* walloc is initialized to libc's realloc, but declared
 * so that we can mock it during tests. */
void *(*walloc)(void *p, size_t size) = realloc;
