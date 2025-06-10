CC=g++
CXX=g++
CFLAGS = -Wall -pedantic -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -pedantic -std=c++11 -g $(INCS)

RANLIB=ranlib

# Separate source files and header files
LIBSRC=Barrier.cpp MapReduceFramework.cpp
HEADERS=Barrier.h
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.

OSMLIB = libMapReduceFramework.a
TARGETS = $(OSMLIB)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex3.tar
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
