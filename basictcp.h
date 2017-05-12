#ifndef BASE_TCP_H
#define BASE_TCP_H

using namespace std;
#include "postal.h"
#include <sys/poll.h>
#include <string>
#include <sys/types.h>
#include <netinet/in.h>

class Logit;
class address;

#ifdef USE_SSL
#ifdef USE_OPENSSL
#ifndef TCP_BODY
struct SSL_METHOD;
struct SSL_CTX;
struct SSL;
struct X509;
#else // TCP_BODY
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif // TCP_BODY
#else // USE_OPENSSL
#include <gnutls/gnutls.h>
#endif // USE_OPENSSL
#endif

class results;

class base_tcp
{
public:
  base_tcp(int fd, Logit *log, Logit *debug, results *res
#ifdef USE_SSL
    , int ssl
#endif
  );
  virtual ~base_tcp();

  int do_stuff();

#ifdef USE_SSL
  // after calling Connect() and getting high-level protocol ready call
  // ConnectTLS() to start TLS.
  int ConnectTLS();
  int disconnect();
  int isTLS() { return m_isTLS; }
#endif

  // returns negative error or the number of bytes read
  int readLine(char *buf, int bufSize, bool stripCR = false, int timeout = 60);

  ERROR_TYPE sendData(CPCCHAR buf, int size);
  ERROR_TYPE sendCstr(CPCCHAR buf) { return sendData(buf, strlen(buf)); }
  ERROR_TYPE sendString(const string &s) { return sendData(s.c_str(), s.size()); }
  ERROR_TYPE printf(CPCCHAR fmt, ...);

protected:

  struct sockaddr_in m_localAddr, m_remoteAddr;
#ifdef USE_SSL
  bool m_canTLS;
  int m_useTLS;
#endif

  void sentData(int bytes);
  virtual void receivedData(int bytes);

private:
  int m_sock;
  pollfd m_poll;
  char m_buf[4096];
  int m_start;
  int m_end;
  bool m_open;
  Logit *m_log;
  // debug file management object (NULL if no debugging)
  // if used a new Logit object is created for each instance unlike the
  // m_log instance
  Logit *m_debug;

  results *m_res;

#ifdef USE_SSL
#ifdef USE_OPENSSL
  SSL_METHOD *m_sslMeth;
  SSL_CTX* m_sslCtx;
  SSL *m_ssl;
#else
  gnutls_session_t m_gnutls_session;
  gnutls_anon_server_credentials_t m_anoncred;
  void m_generate_dh_params();
  void m_initialize_tls_session();
  static gnutls_dh_params_t m_dh_params;
  static int m_init_dh_params;
#endif
  bool m_isTLS;
#endif

  base_tcp(const base_tcp&);
  base_tcp & operator=(const base_tcp&);
};

#endif
