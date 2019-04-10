# flags
CFLAGS = -Wall -Werror -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -g
CPPFLAGS = -DAUKLET_VERSION=\"$(shell cat ../VERSION)\" -DAUKLET_TIMESTAMP=\"$(TIMESTAMP)\"

# default toolchain
OC ?= objcopy
NM ?= nm

# agent library installation path
INSTALL = /usr/local

# Overriden by CircleCI to insert built timestamp
TIMESTAMP ?= 'no timestamp'

# project structure
MOD = buf server json node logger agent
SRC = $(MOD:=.c)
OBJ = $(MOD:=.o)
COVER = buf json node server
GCOV = $(MOD:=.c.gcov) $(COVER:=_test.gcno) $(COVER:=_test.gcda)

# utility functions

define implicitRule =

$(shell $(CC) -MM $1)

endef

# write an implicit rule for each module
moduleRules = $(foreach f,$(SRC),$(call implicitRule,$f))

define writeTest =

	@echo testing $1
	@valgrind --error-exitcode=1 -q ./$1_test
	@gcov $1_test.c
	@rm -f $1_test.c.gcov # don't want coverage on the test apparatus
	@sed -i s,Source:,Source:src/, $1.c.gcov
endef

# test coverage flags
cover = -fprofile-arcs -ftest-coverage

# shell commands for building tests
compileWithCoverage = @$(CC) $(CPPFLAGS) $(cover) $(CFLAGS) -c $<
linkWithCoverage = @$(CC) $(LDFLAGS) $^ $(cover) $(LOADLIBES) $(LDLIBS) -o $@
