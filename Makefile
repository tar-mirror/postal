
PROGRAMS=postal postal-list rabid

all: $(PROGRAMS)

CC=gcc -O2 -g -Wall -pipe -Wshadow -Wpointer-arith -Wwrite-strings -Wcast-align -Wcast-qual -Woverloaded-virtual

BASEOBJS=expand.o userlist.o forkit.o results.o address.o tcp.o cmd5.o mutex.o logit.o
ALLOBJS=$(BASEOBJS) smtp.o pop.o
TESTPROGRAMS=ex-test
LFLAGS=-lstdc++ -lpthread -lssl -lcrypto

postal: postal.cpp $(BASEOBJS) postal.h smtp.o
	$(CC) postal.cpp $(BASEOBJS) smtp.o -o postal $(LFLAGS)

rabid: rabid.cpp $(BASEOBJS) postal.h pop.o
	$(CC) rabid.cpp $(BASEOBJS) pop.o -o rabid $(LFLAGS)

ex-test: ex-test.cpp expand.o
	$(CC) ex-test.cpp expand.o -o ex-test $(LFLAGS)

postal-list: postal-list.cpp expand.o
	$(CC) postal-list.cpp expand.o -o postal-list $(LFLAGS)

install:
	mkdir -p $(DESTDIR)/usr/sbin
	strip $(PROGRAMS)
	cp $(PROGRAMS) $(DESTDIR)/usr/sbin

%.o: %.cpp %.h postal.h
	$(CC) -c $< -o $@

clean:
	rm -f $(PROGRAMS) $(TESTPROGRAMS) $(ALLOBJS) build-stamp install-stamp
	rm -rf debian/tmp err out core debian/*.debhelper debian/{substvars,files}
