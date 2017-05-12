# Generated automatically from Makefile.in by configure.

PROGRAMS=postal postal-list rabid

all: $(PROGRAMS)

CFLAGS=-DDOMAINNAME
CXX=c++ $(CFLAGS) -O2 -g -Wall -pipe -Wshadow -Wpointer-arith -Wwrite-strings -Wcast-align -Wcast-qual -Woverloaded-virtual

TESTPROGRAMS=ex-test
BASEOBJS=expand.o userlist.o forkit.o results.o address.o tcp.o cmd5.o mutex.o logit.o 
LFLAGS=-lstdc++ -lpthread -lssl -lcrypto


ALLOBJS=$(BASEOBJS) smtp.o client.o

postal: postal.cpp $(BASEOBJS) postal.h smtp.o
	$(CXX) postal.cpp $(BASEOBJS) smtp.o -o postal $(LFLAGS)

rabid: rabid.cpp $(BASEOBJS) postal.h client.o
	$(CXX) rabid.cpp $(BASEOBJS) client.o -o rabid $(LFLAGS)

ex-test: ex-test.cpp expand.o
	$(CXX) ex-test.cpp expand.o -o ex-test $(LFLAGS)

postal-list: postal-list.cpp expand.o
	$(CXX) postal-list.cpp expand.o -o postal-list $(LFLAGS)

install:
	mkdir -p $(DESTDIR)/usr/sbin
	strip $(PROGRAMS)
	cp $(PROGRAMS) $(DESTDIR)/usr/sbin

%.o: %.cpp %.h postal.h
	$(CXX) -c $<

clean:
	rm -f $(PROGRAMS) $(TESTPROGRAMS) $(ALLOBJS) build-stamp install-stamp
	rm -rf debian/tmp err out core debian/*.debhelper
	rm -f debian/{substvars,files} config.log

realclean: clean
	rm -f config.*