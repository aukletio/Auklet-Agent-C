include config.mk

SRC = buf.c timer.c json.c node.c socket.c agent.c walloc.c
OBJ = $(SRC:.c=.o)

all: test install

install: libauklet.a
	sudo cp $< $(INSTALL)/$<

uninstall:
	sudo rm -f $(INSTALL)/libauklet.a

libauklet.tgz: libauklet.a
	tar cz -f $(TARNAME) $<

libauklet.a: libauklet.o
	$(AR) rcs $@ $<

# libauklet.o has no internal interfaces exposed. Only the instrumentation
# functions are globally visible.
libauklet.o: libauklet-internal.o internal-symbols.txt
	$(OC) --localize-symbols=internal-symbols.txt $< $@

# internal-symbols.txt lists the symbols of all the internal interfaces, which
# we want to localize.
internal-symbols.txt: libauklet-internal.o
	$(NM) -g --defined-only $< | awk '$$3 !~ /^__cyg_profile_func_e/ {print $$3}' >$@

# libauklet-internal.o links each module into one object file. However, the
# internal interfaces are global symbols, which could lead to naming collisions.
libauklet-internal.o: $(OBJ)
	$(LD) -r $^ -o $@

agent.o: CFLAGS += -DAUKLET_VERSION=\"$$(cat ../VERSION)\" -DAUKLET_TIMESTAMP=\"${TIMESTAMP}\"
agent.o: agent.c timer.h node.h socket.h buf.h json.h

json.o: json.c json.h node.h buf.h

socket.o: socket.c socket.h node.h

# This rule handles modules buf, node, timer and walloc.
%.o: %.c %.h

test: json_test
	./json_test

json_test: json_test.o json.o node.o buf.o walloc.o
	$(CC) -o $@ $^

json_test.o: json_test.c json.h node.h buf.h

clean:
	rm -f $(OBJ) libauklet-internal.o libauklet.o libauklet.a libauklet.tgz internal-symbols.txt json_test json_test.o

.PHONY: install uninstall clean test
