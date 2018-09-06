#include <stdlib.h>

void *(*walloc)(void *p, size_t size) = realloc;
