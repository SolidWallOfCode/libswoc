
SRC=src/ArenaWriter.cc  src/bw_ip_format.cc  src/MemArena.cc  src/string_view_util.cc  src/swoc_ip.cc \
	src/bw_format.cc src/Errata.cc src/RBTree.cc src/swoc_file.cc  src/TextView.cc

OBJS=obj/ArenaWriter.o  obj/bw_ip_format.o  obj/MemArena.o  obj/string_view_util.o  obj/swoc_ip.o \
	obj/bw_format.o obj/Errata.o obj/RBTree.o obj/swoc_file.o  obj/TextView.o

PREFIX := /usr/local

CXX=g++
CFLAGS=-O3 -g -Iinclude -std=c++17 -fPIC

all:	obj libswoc.a

obj/%.o: src/%.cc
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

libswoc.a:	$(OBJS)
	ar rcs $@ $^

obj:
	mkdir obj

install:	libswoc.a
	cp libswoc.a $(PREFIX)/lib/
	cp -r include/* $(PREFIX)/include/

clean:
	rm -f obj/* *.a
	

