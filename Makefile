CFLAGS = -Wall -Werror -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -g

SRC = buf.c emit.c json.c node.c socket.c agent.c
OBJ = $(SRC:.c=.o)

all: $(OBJ)

clean:
	rm -f $(OBJ)

.PHONY: all clean
