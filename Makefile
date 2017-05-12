
PROGRAMS=postal postal-list rabid

all: $(PROGRAMS)

ifdef SSL
CFLAGS=-DUSE_SSL -DLINUX
else
CFLAGS=-DLINUX
endif
CC=g++ $(CFLAGS) -O2 -g -Wall -pipe -Wshadow -Wpointer-arith -Wwrite-strings -Wcast-align -Wcast-qual -Woverloaded-virtual

TESTPROGRAMS=ex-test
ifdef SSL
BASEOBJS=expand.o userlist.o forkit.o results.o address.o tcp.o cmd5.o mutex.o logit.o
LFLAGS=-lstdc++ -lpthread -lssl -lcrypto
else
BASEOBJS=expand.o userlist.o forkit.o results.o address.o tcp.o cmd5.o mutex.o logit.o md5.o
LFLAGS=-lstdc++ -lpthread -lsocket -lnsl
endif
ALLOBJS=$(BASEOBJS) smtp.o client.o

postal: postal.cpp $(BASEOBJS) postal.h smtp.o
	$(CC) postal.cpp $(BASEOBJS) smtp.o -o postal $(LFLAGS)

rabid: rabid.cpp $(BASEOBJS) postal.h client.o
	$(CC) rabid.cpp $(BASEOBJS) client.o -o rabid $(LFLAGS)

ex-test: ex-test.cpp expand.o
	$(CC) ex-test.cpp expand.o -o ex-test $(LFLAGS)

postal-list: postal-list.cpp expand.o
	$(CC) postal-list.cpp expand.o -o postal-list $(LFLAGS)

install:
	mkdir -p $(DESTDIR)/usr/sbin
	strip $(PROGRAMS)
	cp $(PROGRAMS) $(DESTDIR)/usr/sbin

%.o: %.cpp %.h postal.h
	$(CC) -c $<

clean:
	rm -f $(PROGRAMS) $(TESTPROGRAMS) $(ALLOBJS) build-stamp install-stamp
	rm -rf debian/tmp err out core debian/*.debhelper debian/{substvars,files}
