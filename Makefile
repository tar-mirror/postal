# Generated automatically from Makefile.in by configure.
EXE=postal postal-list rabid
MAN8=postal-list.8 postal.8 rabid.8

all: $(EXE)

prefix=/home/rjc/debian/postal-0.61/debian/postal/usr
eprefix=${prefix}
WFLAGS=-Wall -W -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wcast-qual -Woverloaded-virtual -pedantic -ffor-scope

CXX=c++ $(CFLAGS) -O2 -g $(WFLAGS)
CC=gcc $(CFLAGS) -O2 -g $(WFLAGS)

INSTALL=/usr/bin/install -c

TESTEXE=ex-test
BASEOBJS=expand.o userlist.o thread.o results.o address.o tcp.o cmd5.o mutex.o logit.o 
LFLAGS=-lstdc++  -lssl -lcrypto -lpthread


ALLOBJS=$(BASEOBJS) smtp.o client.o

postal: postal.cpp $(BASEOBJS) postal.h smtp.o
	$(CXX) postal.cpp $(BASEOBJS) smtp.o -o postal $(LFLAGS)

rabid: rabid.cpp $(BASEOBJS) postal.h client.o
	$(CXX) rabid.cpp $(BASEOBJS) client.o -o rabid $(LFLAGS)

ex-test: ex-test.cpp expand.o
	$(CXX) ex-test.cpp expand.o -o ex-test $(LFLAGS)

postal-list: postal-list.cpp expand.o
	$(CXX) postal-list.cpp expand.o -o postal-list $(LFLAGS)

install-bin: $(EXE)
	mkdir -p $(eprefix)/sbin
	${INSTALL} -s $(EXE) $(eprefix)/sbin

install: install-bin
	mkdir -p ${prefix}/man/man8
	${INSTALL} -m 644 $(MAN8) ${prefix}/man/man8

%.o: %.cpp %.h postal.h
	$(CXX) -c $<

%.o: %.c %.h
	$(CC) -c $<

clean:
	rm -f $(EXE) $(TESTEXE) $(ALLOBJS) md5.o build-stamp install-stamp
	rm -rf debian/tmp core debian/*.debhelper
	rm -f debian/{substvars,files} config.log

realclean: clean
	rm -f config.* Makefile postal.spec sun/pkginfo
