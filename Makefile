SEXE=postal rabid bhm
EXE=postal-list
MAN8=postal.8 rabid.8 bhm.8
MAN1=postal-list.1

all: $(EXE) $(SEXE)

prefix=/home/rjc/postal/postal-0.66/debian/postal/usr
eprefix=${prefix}
WFLAGS=-Wall -W -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wcast-qual -Woverloaded-virtual -pedantic -ffor-scope

CXX=g++ $(CFLAGS) -O2 -g $(WFLAGS)
CC=gcc $(CFLAGS) -O2 -g $(WFLAGS)

INSTALL=/usr/bin/install -c

TESTEXE=ex-test
BASEOBJS=userlist.o thread.o results.o address.o tcp.o cmd5.o mutex.o logit.o expand.o md5.o
LFLAGS=-lstdc++  -lgnutls -lpthread


ALLOBJS=$(BASEOBJS) smtp.o client.o basictcp.o bhmusers.o smtpserver.o

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

dep:
	makedepend -Y *.cpp *.h
	makedepend -f Makefile.in -Y *.cpp *.h

clean:
	rm -f $(EXE) $(SEXE) $(TESTEXE) $(ALLOBJS) md5.o build-stamp install-stamp
	rm -rf debian/tmp core debian/*.debhelper
	rm -f debian/{substvars,files} config.log

realclean: clean
	rm -f config.* Makefile postal.spec sun/pkginfo
# DO NOT DELETE

address.o: address.h
basictcp.o: basictcp.h postal.h userlist.h conf.h address.h logit.h results.h
basictcp.o: mutex.h
bhm.o: bhmusers.h conf.h postal.h logit.h results.h mutex.h basictcp.h
bhmusers.o: bhmusers.h conf.h postal.h expand.h
client.o: client.h tcp.h postal.h cmd5.h md5.h thread.h port.h userlist.h
client.o: conf.h logit.h results.h mutex.h
cmd5.o: postal.h cmd5.h md5.h
expand.o: expand.h
ex-test.o: expand.h
logit.o: logit.h
mutex.o: postal.h mutex.h
postal.o: userlist.h conf.h postal.h smtp.h tcp.h cmd5.h md5.h thread.h
postal.o: port.h mutex.h logit.h
postal-list.o: expand.h
rabid.o: userlist.h conf.h postal.h client.h tcp.h cmd5.h md5.h thread.h
rabid.o: port.h logit.h
results.o: postal.h results.h mutex.h
smtp.o: smtp.h conf.h tcp.h postal.h cmd5.h md5.h thread.h port.h mutex.h
smtp.o: userlist.h logit.h results.h
smtpserver.o: smtpserver.h logit.h tcp.h postal.h cmd5.h md5.h thread.h
smtpserver.o: port.h results.h mutex.h
tcp.o: tcp.h postal.h cmd5.h md5.h thread.h port.h userlist.h conf.h
tcp.o: address.h logit.h
thread.o: thread.h port.h
userlist.o: userlist.h conf.h postal.h expand.h
basictcp.o: postal.h
bhmusers.o: conf.h postal.h
client.o: tcp.h postal.h cmd5.h md5.h thread.h port.h
cmd5.o: md5.h
results.o: mutex.h
smtp.o: conf.h tcp.h postal.h cmd5.h md5.h thread.h port.h mutex.h
smtpserver.o: logit.h tcp.h postal.h cmd5.h md5.h thread.h port.h results.h
smtpserver.o: mutex.h
tcp.o: postal.h cmd5.h md5.h thread.h port.h
thread.o: port.h
userlist.o: conf.h postal.h
