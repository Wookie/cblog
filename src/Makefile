# build cblog
#

# configurable variables
CUTIL_ROOT?=../cutil/src
CLLSD_ROOT?=../cllsd/src
EXTRA_LIBS_ROOT?=/usr/local
PREFIX?=/usr/local
COVERAGE?=../coverage

SHELL=/bin/sh
NAME=cblog
SRC=cblog.c misc.c
HDR=
OBJ=$(SRC:.c=.o)
OUT=$(NAME)
LIBOUT=lib$(NAME).a
GCDA=$(SRC:.c=.gcda)
GCNO=$(SRC:.c=.gcno)
GCOV=$(SRC:.c=.c.gcov)
# set CFLAGS to something gcov can use
LIBS=-lcllsd -lcutil -lcunit -lexpat -lev -lm
CFLAGS=-O0 -gstabs+ -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include -I$(EXTRA_LIBS_ROOT)/include
LDFLAGS=-gstabs+ -L$(CLLSD_ROOT)/lib -L$(CUTIL_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib
coverage: LIBS=-lcllsd -lcutil -lcunit -lexpat -lev -lm -lgcov
coverage: CFLAGS=-fprofile-arcs -ftest-coverage -O0 -gstabs+ -DUNIT_TESTING -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include -I$(EXTRA_LIBS_ROOT)/include
test: CFLAGS=-O0 -gstabs+ -DUNIT_TESTING -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include -I$(EXTRA_LIBS_ROOT)/include


# install utility
INSTALL=/usr/bin/install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644

all: $(OUT)

lib_all: $(LIBOUT)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(LIBOUT): $(OBJ)
	ar rcs $(LIBOUT) $(OBJ)
	rm -rf $(NAME)
	rm -rf include
	rm -rf lib
	ln -s . $(NAME)
	ln -s . include
	ln -s . lib

install: all
	mkdir -p $(PREFIX)/bin
	$(INSTALL_PROGRAM) --target-directory $(PREFIX)/bin/ $(OUT)

uninstall:
	rm -rf $(PREFIX)/bin/$(OUT)

test: lib_all

coverage: lib_all

report:
	lcov --base-directory . --directory . -c -o $(NAME).info
	lcov --remove $(NAME).info "/usr*" -o $(NAME).info
	rm -rf $(COVERAGE)
	genhtml -o $(COVERAGE) -t "cutil test coverage" --num-spaces 4 $(NAME).info

clean:
	rm -rf $(OBJ)
	rm -rf include
	rm -rf lib
	rm -rf $(OUT)
	rm -rf $(LIBOUT)
	rm -rf $(GCDA)
	rm -rf $(GCNO)
	rm -rf $(GCOV)
	rm -rf ev.h.gcov

.PHONY: all install uninstall clean test coverage report

