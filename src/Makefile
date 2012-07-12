# build cblog
#

# configurable variables
CUTIL_ROOT?=../cutil/src
CLLSD_ROOT?=../cllsd/src
EXTRA_LIBS_ROOT?=/usr/local
PREFIX?=/usr/local

SHELL=/bin/sh
SRC=cblog.c
HDR=
OBJ=$(SRC:.c=.o)
OUT=cblog
LIBS=-lcllsd -lcutil -lcunit -lexpat -lev
CFLAGS=-O0 -gstabs+ -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include -I$(EXTRA_LIBS_ROOT)/include
LDFLAGS=-gstabs+ -L$(CLLSD_ROOT)/lib -L$(CUTIL_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib

# install utility
INSTALL=/usr/bin/install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644

all: $(OUT)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

install: all
	mkdir -p $(PREFIX)/bin
	$(INSTALL_PROGRAM) --target-directory $(PREFIX)/bin/ $(OUT)

uninstall:
	rm -rf $(PREFIX)/bin/$(OUT)

test: all

clean:
	rm -rf $(OBJ)
	rm -rf $(OUT)

.PHONY: all install uninstall clean test

