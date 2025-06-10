CC=g++
CXX=g++
CFLAGS = -Wall -pedantic -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -pedantic -std=c++11 -g $(INCS)

RANLIB=ranlib

# Separate source files and header files
LIBSRC=uthreads.cpp user_thread.cpp
HEADERS=user_thread.h
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.

OSMLIB = libuthreads.a
TARGETS = $(OSMLIB)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex2.tar
TARSRCS=$(LIBSRC) $(HEADERS) Makefile README

all: $(TARGETS)

$(TARGETS): $(LIBOBJ)
	@ar rcs $@ $^

clean:
	$(RM) $(TARGETS) $(LIBOBJ) *~ *core

depend:
	makedepend -- $(CFLAGS) -- $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
