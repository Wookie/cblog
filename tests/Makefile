# build cblog test executable
#

# configurable variables
CUTIL_ROOT?=../cutil/src
CLLSD_ROOT?=../cllsd/src
EXTRA_LIBS_ROOT?=/usr/local

SHELL=/bin/sh
SRC=test_all.c
OBJ=$(SRC:.c=.o)
GCDA=$(SRC:.c=.gcda)
GCNO=$(SRC:.c=.gcno)
GCOV=$(SRC:.c=.c.gcov)
OUT=test_all
LIBS=-lcblog -lcllsd -lcutil -lcunit -lexpat -lev
CBLOG_ROOT=../src
CFLAGS=-O0 -gstabs+ -I$(CBLOG_ROOT)/include -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include -I$(EXTRA_LIBS_ROOT)/include
LDFLAGS=-O0 -gstabs+ -L$(CBLOG_ROOT)/lib -L$(CLLSD_ROOT)/lib -L$(CUTIL_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib
# set LDFLAGS to something gcov can use
coverage: LDFLAGS=-fprofile-arcs -O0 -gstabs+ -L$(CBLOG_ROOT)/lib -L$(CLLSD_ROOT)/lib -L$(CUTIL_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib


all:

test: $(OUT)
	./test_all

coverage: $(OUT)
	./test_all

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

install:

uninstall:

report:

clean:
	rm -rf $(OBJ)
	rm -rf $(OUT)
	rm -rf $(GCDA)
	rm -rf $(GCNO)
	rm -rf $(GCOV)

.PHONY: all install uninstall clean test coverage report

