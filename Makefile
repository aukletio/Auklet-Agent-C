CFLAGS = -Wall -Werror -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -g

SRC = buf.c timer.c json.c node.c socket.c agent.c
OBJ = $(SRC:.c=.o)

# libauklet.o has no internal interfaces exposed. Only the instrumentation
# functions are globally visible.
libauklet.o: libauklet-internal.o internal-symbols.txt
	objcopy --localize-symbols=internal-symbols.txt $< $@

# internal-symbols.txt lists the symbols of all the internal interfaces, which
# we want to localize.
internal-symbols.txt: libauklet-internal.o
	nm -g --defined-only $< | awk '$$3 !~ /^__cyg_profile_func_e/ {print $$3}' >$@

# libauklet-internal.o links each module into one object file. However, the
# internal interfaces are global symbols, which could lead to naming collisions.
libauklet-internal.o: $(OBJ)
	$(LD) -r $^ -o $@

# This rule handles modules buf, node, and timer.
%.o: %.c %.h

json.o: json.c json.h node.h buf.h

socket.o: socket.c node.h

agent.o: CFLAGS += -DAUKLET_VERSION=\"$$(cat VERSION)\" -DAUKLET_TIMESTAMP=\"${TIMESTAMP}\"
agent.o: agent.c timer.h node.h socket.h buf.h json.h

clean:
	rm -f $(OBJ) libauklet-internal.o libauklet.o internal-symbols.txt

.PHONY: all clean
