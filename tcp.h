#ifndef TCP_H
#define TCP_H

// Misnamed really.  This class does all the common functionality between SMTP
// and POP clients.

#include "postal.h"
#include <sys/poll.h>
#include <string>
#include "cmd5.h"
#include "thread.h"
#include <netinet/in.h>

class Logit;
class address;

#ifndef TCP_BODY
struct SSL_METHOD;
struct SSL_CTX;
struct SSL;
struct X509;
#endif

class tcp : public Thread
{
public:
  tcp(const char *addr, unsigned short default_port, Logit *log
#ifdef USE_SSL
    , int ssl
#endif
    , const char *sourceAddr = NULL);
  tcp(int threadNum, const tcp *parent);
  virtual ~tcp();

  // connect returns 0 for connect, 1 for can't connect, and 2 for serious
  // errors.
  int connect(short port = 0);
#ifdef USE_SSL
  // after calling connect() and getting high-level protocol ready call
  // connectTLS() to start TLS.
  int connectTLS();
#endif
  int sendMsg();
  virtual int disconnect();
  int doAllWork(int rate);

protected:
  virtual int pollRead() = 0;
  virtual int WriteWork(PVOID buf, int size, int timeout) = 0;

  int readLine(char *buf, int bufSize);
  int sendData(const char *buf, int size);
  int sendString(const string &s) { return sendData(s.c_str(), s.size()); }
  void endIt();
  virtual void error() = 0;
  virtual void sentData(int bytes) = 0;
  virtual void receivedData(int bytes) = 0;
  Cmd5 m_md5;

  virtual int readCommandResp(bool important = true) = 0;
  int sendCommandData(const char *buf, int size, bool important = true);
  virtual int sendCommandString(const string &s, bool important = true);

  int m_destAffinity;
  Logit *m_log;
  struct sockaddr_in m_connectionSourceAddr;
#ifdef USE_SSL
  int m_useTLS;
#endif

private:
  int m_fd;
  pollfd m_poll;
  char m_buf[4096];
  int m_start;
  int m_end;
  bool m_open;
  address *m_addr; // destination
  address *m_sourceAddr;
#ifdef USE_SSL
  SSL_METHOD *m_sslMeth;
  SSL_CTX* m_sslCtx;
  SSL *m_ssl;
  bool m_isTLS;
#endif
};

#endif
