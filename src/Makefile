include config.mk

all: test install

$(eval $(moduleRules))

install: libauklet.a
	sudo cp $< $(INSTALL)/$<

uninstall:
	sudo rm -f $(INSTALL)/libauklet.a

libauklet.tgz: libauklet.a
	tar cz -f $(TARNAME) $< ../LICENSE

libauklet.a: libauklet.o
	$(AR) rcs $@ $<

# libauklet.o has all internal interfaces prefixed. Only the instrumentation
# functions are not prefixed.
libauklet.o: libauklet-internal.o internal-symbols.txt
	$(OC) --redefine-syms=internal-symbols.txt $< $@

# internal-symbols.txt lists the symbols of all the internal interfaces, which,
# to avoid naming collisions, we want to prefix.
internal-symbols.txt: libauklet-internal.o
	$(NM) -g --defined-only $< | awk '$$3 !~ /^__cyg_profile_func_e/ {printf "%s libauklet_%s\n",$$3,$$3}' >$@

# libauklet-internal.o links each module into one object file. However, the
# internal interfaces are global symbols, which could lead to naming collisions.
libauklet-internal.o: $(OBJ)
	$(LD) -r $^ -o $@

test: $(TEST:=_test)
	$(foreach t,$(TEST),$(call writeTest,$t))

node_test: node_test.o walloc.o
	$(linkWithCoverage)

node_test.o: node_test.c node.h walloc.h
	$(compileWithCoverage)

json_test: json_test.o node.o buf.o walloc.o
	$(linkWithCoverage)

json_test.o: json_test.c json.h node.h buf.h
	$(compileWithCoverage)

buf_test: buf_test.o walloc.o
	$(linkWithCoverage)

buf_test.o: buf_test.c walloc.h
	$(compileWithCoverage)

server_test: server_test.o
	$(linkWithCoverage) -lpthread

server_test.o: server_test.c server.h
	$(compileWithCoverage) -lpthread

clean:
	@rm -f $(OBJ)
	@rm -f $(TEST:=_test) $(TEST:=_test.o)
	@rm -f libauklet-internal.o libauklet.o libauklet.a libauklet.tgz internal-symbols.txt
	@rm -f $(GCOV)

.PHONY: install uninstall clean test