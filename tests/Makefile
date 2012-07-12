# build cblog test executable
#

# configurable variables
CUTIL_ROOT?=../cutil/src
CLLSD_ROOT?=../cllsd/src
EXTRA_LIBS_ROOT?=/usr/local

SHELL=/bin/sh
SRC=test_all.c
OBJ=$(SRC:.c=.o)
OUT=test_all
LIBS=-lcllsd -lcutil -lcunit -lexpat -lev
CBOT_ROOT=../src
CFLAGS=-O0 -gstabs+ -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include -I$(EXTRA_LIBS_ROOT)/include
LDFLAGS=-gstabs+ -L$(CLLSD_ROOT)/lib -L$(CUTIL_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib

all:

test: $(OUT)
	./test_all

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

install:

uninstall:

clean:
	rm -rf $(OBJ)
	rm -rf $(OUT)

.PHONY: all install uninstall clean test

