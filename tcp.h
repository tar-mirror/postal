#ifndef TCP_H
#define TCP_H

// Misnamed really.  This class does all the common functionality between SMTP
// and POP clients.

using namespace std;
#include "postal.h"
#include <sys/poll.h>
#include <string>
#include "cmd5.h"
#include "thread.h"
#include <sys/types.h>
#include <netinet/in.h>

class Logit;
class address;

#ifdef USE_SSL
#ifndef TCP_BODY
struct SSL_METHOD;
struct SSL_CTX;
struct SSL;
struct X509;
#else
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif
#endif

class tcp : public Thread
{
public:
  tcp(const char *addr, unsigned short default_port, Logit *log
#ifdef USE_SSL
    , int ssl
#endif
    , const char *sourceAddr, Logit *debug);
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
  ERROR_TYPE sendData(CPCCHAR buf, int size);
  ERROR_TYPE sendString(const string &s) { return sendData(s.c_str(), s.size()); }
  void endIt();
  virtual void error() = 0;
  virtual void sentData(int bytes) = 0;
  virtual void receivedData(int bytes) = 0;
  Cmd5 m_md5;

  virtual ERROR_TYPE readCommandResp(bool important = true) = 0;
  virtual ERROR_TYPE sendCommandData(const char *buf, int size, bool important = true);
  virtual ERROR_TYPE sendCommandString(const string &str, bool important = true);

  int m_destAffinity;
  Logit *m_log;
  struct sockaddr_in m_connectionSourceAddr;
#ifdef USE_SSL
  bool m_canTLS;
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
  Logit *m_debug; // debug file management object (NULL if no debugging)
#ifdef USE_SSL
  SSL_METHOD *m_sslMeth;
  SSL_CTX* m_sslCtx;
  SSL *m_ssl;
  bool m_isTLS;
#endif

  tcp(const tcp&);
  tcp & operator=(const tcp&);
};

#endif
