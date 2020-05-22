
AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

bin_PROGRAMS = example

example_SOURCES= example.c ../src/scqd.c

example_CPPFLAGS  = $(DEPS_CFLAGS) -I../src
example_CFLAGS  = $(PTHREAD_CFLAGS)
example_LDFLAGS = $(PTHREAD_LDFLAGS)
example_LDADD  = $(DEPS_LIBS)
