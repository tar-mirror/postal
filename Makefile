SEXE=postal rabid bhm
EXE=postal-list
MAN8=postal.8 rabid.8 bhm.8
MAN1=postal-list.1

all: $(EXE) $(SEXE)

prefix=/usr/local
eprefix=${prefix}
WFLAGS=-Wall -W -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wcast-qual -Woverloaded-virtual -pedantic -ffor-scope

CXX=g++ $(CFLAGS) -O2 -g $(WFLAGS)
CC=gcc $(CFLAGS) -O2 -g $(WFLAGS)

INSTALL=/usr/bin/install -c

TESTEXE=ex-test
BASEOBJS=userlist.o thread.o results.o address.o tcp.o cmd5.o mutex.o logit.o expand.o md5.o
LFLAGS=-lstdc++  -lgnutls -lpthread


ALLOBJS=$(BASEOBJS) smtp.o client.o basictcp.o bhmusers.o

postal: postal.cpp $(BASEOBJS) postal.h smtp.o
	$(CXX) postal.cpp $(BASEOBJS) smtp.o -o postal $(LFLAGS)

rabid: rabid.cpp $(BASEOBJS) postal.h client.o
	$(CXX) rabid.cpp $(BASEOBJS) client.o -o rabid $(LFLAGS)

bhm: bhm.cpp userlist.o basictcp.o logit.o results.o mutex.o bhmusers.o postal.h
	$(CXX) bhm.cpp userlist.o basictcp.o logit.o results.o mutex.o bhmusers.o -o bhm $(LFLAGS)

ex-test: ex-test.cpp expand.o
	$(CXX) ex-test.cpp expand.o -o ex-test $(LFLAGS)

postal-list: postal-list.cpp expand.o
	$(CXX) postal-list.cpp expand.o -o postal-list $(LFLAGS)

install-bin: $(EXE) $(SEXE)
	mkdir -p $(eprefix)/sbin $(eprefix)/bin
	${INSTALL} -s $(SEXE) $(eprefix)/sbin
	${INSTALL} -s $(EXE) $(eprefix)/bin

install: install-bin
	mkdir -p ${prefix}/share/man/man8 ${prefix}/share/man/man1
	${INSTALL} -m 644 $(MAN8) ${prefix}/share/man/man8
	${INSTALL} -m 644 $(MAN1) ${prefix}/share/man/man1

%.o: %.cpp %.h postal.h
	$(CXX) -c $<

%.o: %.c %.h
	$(CC) -c $<

clean:
	rm -f $(EXE) $(SEXE) $(TESTEXE) $(ALLOBJS) md5.o build-stamp install-stamp
	rm -rf debian/tmp core debian/*.debhelper
	rm -f debian/{substvars,files} config.log

realclean: clean
	rm -f config.* Makefile postal.spec sun/pkginfo
