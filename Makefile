#
# Makefile for lua-shell
#
# Copyright (c) 2015, Wing Eng
#

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
  CINCLUDES=-I/usr/include/lua5.1 -I/usr/local/include -I../linenoise
  LIBS=-L/usr/local/lib -llua
endif

ifeq ($(UNAME_S), Linux)
  CINCLUDES=-I/usr/include/lua5.1 -I../linenoise
  LIBS=-L/usr/lib/x86_64-linux-gnu -llua5.1
endif


CFLAGS=-g -std=gnu99 ${CINCLUDES}
CXXFLAGS =-g -Wall -std=c++0x ${CINCLUDES}

OBJS = \
	lua-shell.o \
	lua-shell-completion.o \
	parse-args.o \
	spawn.o \
	str-split.o

PROGS=lshell

all: ${PROGS}
	@echo "done"

spawn: strlib.a spawn.cpp
	${CXX} ${CXX_FLAGS} -D_TEST -o spawn spawn.cpp strlib.a

lshell:	${OBJS} ../linenoise/linenoise.a
	${CXX} ${CXXFLAGS} -o lshell ${OBJS} ${LIBS} ../linenoise/linenoise.a 

clean:
	rm -f *.o *.a ${PROGS} 
	rm -rf *.dSYM
