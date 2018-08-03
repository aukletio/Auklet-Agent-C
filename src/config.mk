# flags
CFLAGS = -Wall -Werror -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -g
CFLAGS += -DAUKLET_VERSION=\"$$(cat ../VERSION)\" -DAUKLET_TIMESTAMP=\"${TIMESTAMP}\"

# default toolchain
OC ?= objcopy
NM ?= nm

# agent library installation path
INSTALL = /usr/local/lib

# Overriden by CircleCI to insert built timestamp
TIMESTAMP ?= 'no timestamp'
