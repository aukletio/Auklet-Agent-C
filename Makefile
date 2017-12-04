include config.mk

all: go x lib_test

go:
	go install ./wrap
	go install ./release

# instrumented, stripped test program
x: x-dbg
	cp $< $@
	strip $@

# instrumented, debuggable test program
x-dbg: x.o rt.o
	${CC} -o $@ $^ ${PLIBS}

# uninstrumented test program
x-raw: x.c
	${CC} -o $@ $< ${CFLAGS} -lpthread

x.o: x.c
	${CC} -o $@ -c ${CFLAGS} ${PFLAGS} $<

lib_test: lib_test.c lib.c
	${CC} -o $@ ${CFLAGS} -g -lpthread $<

rt.o: rt.c lib.c
	${CC} -o $@ -c ${CFLAGS} -DAUKLET_VERSION=\"$$(cat VERSION)\" -DAUKLET_TIMESTAMP=\"${TIMESTAMP}\" $<

libauklet.a: rt.o
	${AR} rcs $@ $<

libauklet.tgz: libauklet.a
	tar cz -f ${TARNAME} $<

install: libauklet.a
	sudo cp $< ${INSTALL}/$<

uninstall:
	sudo rm -f ${INSTALL}/libauklet.a

clean:
	rm -f x x-raw x-dbg x.o rt.o lib_test libauklet.a libauklet.tgz

.PHONY: all clean install uninstall
