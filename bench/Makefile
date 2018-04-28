CFLAGS = -finstrument-functions -Wall -Werror -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -g
LDLIBS = -lauklet -lpthread -lrt

test: overhead
	./overhead 2>/dev/null

clean:
	rm -f overhead

.PHONY: test clean
