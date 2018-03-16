CFLAGS = -Wall -Werror -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -g

clean:
	rm -f node.o buf.o json.o
