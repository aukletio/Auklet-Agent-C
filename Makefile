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
	gcc -o $@ $^ ${PLIBS}

# uninstrumented test program
x-raw: x.c
	gcc -o $@ $< ${CFLAGS} -lpthread

x.o: x.c
	gcc -o $@ -c ${CFLAGS} ${PFLAGS} $<

lib_test: lib_test.c lib.c
	gcc -o $@ ${CFLAGS} -g -lpthread $<

rt.o: rt.c lib.c
	gcc -o $@ -c ${CFLAGS} $<

libauklet.a: rt.o
	ar rcs $@ $<

install: libauklet.a
	sudo cp $< ${INSTALL}/$<

uninstall:
	sudo rm -f ${INSTALL}/libauklet.a

clean:
	rm -f x x-raw x-dbg x.o rt.o lib_test libauklet.a

.PHONY: all clean install uninstall
