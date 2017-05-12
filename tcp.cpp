#define TCP_BODY

#include "tcp.h"

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include "postal.h"
#include "userlist.h"
#include "address.h"
#include "logit.h"

tcp::tcp(int *exitCount, const char *addr, unsigned short default_port
       , Logit *log
#ifdef USE_SSL
       , int ssl
#endif
       , const char *sourceAddr, Logit *debug)
 : m_md5()
 , m_destAffinity(0)
 , m_log(log)
#ifdef USE_SSL
 , m_canTLS(false)
 , m_useTLS(ssl)
#endif
 , m_exitCount(exitCount)
 , m_fd(-1)
 , m_start(0)
 , m_end(0)
 , m_open(false)
 , m_addr(new address(addr, default_port))
 , m_sourceAddr(NULL)
 , m_debug(debug)
#ifdef USE_SSL
#ifdef USE_OPENSSL
 , m_sslMeth(NULL)
 , m_sslCtx(NULL)
 , m_ssl(NULL)
#else
 , m_gnutls_session(NULL)
 , m_gnutls_anoncred(NULL)
#endif
 , m_isTLS(false)
#endif
{
  if(sourceAddr)
    m_sourceAddr = new address(sourceAddr);
  m_destAffinity = getThreadNum() % m_addr->addressCount();
#ifdef USE_SSL
  if(m_useTLS)
  {
#ifdef USE_OPENSSL
//don't seem to need this    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
#endif
  }
#endif
}

tcp::tcp(int threadNum, const tcp *parent)
 : Thread(threadNum, parent)
 , m_md5()
 , m_destAffinity(parent->m_destAffinity)
 , m_log(parent->m_log)
#ifdef USE_SSL
 , m_canTLS(false)
 , m_useTLS(parent->m_useTLS)
#endif
 , m_exitCount(parent->m_exitCount)
 , m_fd(-1)
 , m_start(0)
 , m_end(0)
 , m_open(false)
 , m_addr(parent->m_addr)
 , m_sourceAddr(parent->m_sourceAddr)
 , m_debug(parent->m_debug ? new Logit(*(parent->m_debug), threadNum) : NULL)
#ifdef USE_SSL
#ifdef USE_OPENSSL
 , m_sslMeth(NULL)
 , m_sslCtx(NULL)
 , m_ssl(NULL)
#else
 , m_gnutls_session(NULL)
 , m_gnutls_anoncred(NULL)
#endif
 , m_isTLS(false)
#endif
{
}

tcp::~tcp()
{
  disconnect();
  if(getThreadNum() < 1)
  {
    delete m_addr;
    delete m_sourceAddr;
  }
  if(m_debug)
    delete m_debug;
}

int tcp::Connect(short port)
{
  if(*m_exitCount)
    return 1;
#ifdef USE_SSL
  m_canTLS = false;
#endif
  m_start = 0;
  m_end = 0;
#ifdef USE_SSL
  m_isTLS = false;
#endif
  sockaddr *sa;
  sa = m_addr->get_addr(m_destAffinity);
  if(!sa)
    return 1;
  m_fd = socket(PF_INET, SOCK_STREAM, 0);
  if(m_fd < 0)
  {
    fprintf(stderr, "Can't open socket.\n");
    error();
    return 2;
  }
  int rc;
  if(m_sourceAddr)
  {
    sockaddr *source;
    source = (sockaddr *)m_sourceAddr->get_rand_addr();
    rc = bind(m_fd, source, sizeof(struct sockaddr_in));
    if(rc)
    {
      fprintf(stderr, "Can't bind to port.\n");
      error();
      close(m_fd);
      return 2;
    }
  }
  m_poll.fd = m_fd;
  if(port)
  {
    struct sockaddr_in newAddr;
    memcpy(&newAddr, sa, sizeof(newAddr));
    newAddr.sin_port = htons(port);
    rc = connect(m_fd, (sockaddr *)&newAddr, sizeof(struct sockaddr_in));
  }
  else
  {
    rc = connect(m_fd, sa, sizeof(struct sockaddr_in));
  }
  if(rc)
  {
    fprintf(stderr, "Can't connect to %s port %d.\n"
                  , inet_ntoa(((sockaddr_in *)sa)->sin_addr)
                  , int(ntohs(((sockaddr_in *)sa)->sin_port)) );
    error();
    close(m_fd);
    return 1;
  }
  socklen_t namelen = sizeof(m_connectionLocalAddr);
  rc = getsockname(m_fd, (struct sockaddr *)&m_connectionLocalAddr, &namelen);
  if(rc)
    fprintf(stderr, "Can't getsockname!\n");
  if(m_debug)
    m_debug->reopen();
  m_open = true;
  return 0;
}

#ifdef USE_SSL
int tcp::ConnectTLS()
{
#ifdef USE_OPENSSL
  m_sslCtx = NULL;
  m_ssl = NULL;
  m_sslMeth = SSLv2_client_method();
  if(m_sslMeth == NULL)
  {
    fprintf(stderr, "Can't get SSLv2_client_method.\n");
    error();
    return 2;
  }
  m_sslCtx = SSL_CTX_new(m_sslMeth);
  if(m_sslCtx == NULL)
  {
    fprintf(stderr, "Can't SSL_CTX_new\n");
    error();
    return 2;
  }
  if((m_ssl = SSL_new(m_sslCtx)) == NULL)
  {
    fprintf(stderr, "Can't SSL_new\n");
    SSL_CTX_free(m_sslCtx);
    error();
    return 2;
  }
  SSL_set_fd(m_ssl, m_fd);
  if(-1 == SSL_connect(m_ssl))
  {
    fprintf(stderr, "Can't SSL_CONNECT\n");
    SSL_free(m_ssl);
    SSL_CTX_free(m_sslCtx);
    error();
    return 1;
  }
#else
  gnutls_anon_allocate_client_credentials(&m_gnutls_anoncred);
  m_gnutls_session = new gnutls_session_t;
  // Initialize TLS session
  gnutls_init (m_gnutls_session, GNUTLS_CLIENT);
  // Use default priorities
  gnutls_set_default_priority(*m_gnutls_session); // bug in gnutls interface?
  // Need to enable anonymous specifically
  gnutls_priority_set_direct(*m_gnutls_session, "NORMAL:+ANON-DH", NULL);
  // put the anonymous credentials to the current session
  gnutls_credentials_set(*m_gnutls_session, GNUTLS_CRD_ANON, m_gnutls_anoncred);
  gnutls_transport_set_ptr(*m_gnutls_session, (gnutls_transport_ptr_t)m_fd);

  // Perform the TLS handshake
  if(gnutls_handshake(*m_gnutls_session) < 0)
  {
    fprintf(stderr, "Can't gnutls_handshake\n");
    gnutls_deinit(*m_gnutls_session);
    gnutls_anon_free_client_credentials(m_gnutls_anoncred);
    error();
    return 1;
  }
#endif
  if(*m_exitCount > 1)
    return 3;
  m_isTLS = true;

#if 0
// openssl debugging code that may be useful to have around
// in a commented-out state.
  /* Following two steps are optional and not required for
     data exchange to be successful. */
 
  /* Get the cipher - opt */
 
  printf ("SSL connection using %s\n", SSL_get_cipher(m_ssl));
 
  /* Get server's certificate (note: beware of dynamic allocation) - opt */
 
  X509 *server_cert;
  server_cert = SSL_get_peer_certificate(m_ssl);
  if(!server_cert)
  {
    fprintf(stderr, "Can't SSL_get_peer_certificate\n");
    return 2;
  }
  printf ("Server certificate:\n");
 
  char *str = X509_NAME_oneline(X509_get_subject_name(server_cert),0,0);
  if(!str)
  {
    fprintf(stderr, "Can't X509_NAME_oneline\n");
    return 2;
  }
  printf ("\t subject: %s\n", str);
  Free (str);
  str = X509_NAME_oneline (X509_get_issuer_name(server_cert),0,0);
  if(!str)
  {
    fprintf(stderr, "Can't X509_get_issuer_name\n");
    return 2;
  }
  printf ("\t issuer: %s\n", str);
  Free (str);
 
  /* We could do all sorts of certificate verification stuff here before
     deallocating the certificate. */
 
  X509_free(server_cert);
#endif 
  return 0;
}
#endif

int tcp::disconnect()
{
  if(m_open)
  {
#ifdef USE_SSL
    if(m_isTLS)
    {
#ifdef USE_OPENSSL
      SSL_shutdown(m_ssl);
      close(m_fd);
      SSL_free(m_ssl);
      SSL_CTX_free(m_sslCtx);
#else
      gnutls_bye(*m_gnutls_session, GNUTLS_SHUT_RDWR);
      close(m_fd);
      gnutls_deinit(*m_gnutls_session);
      gnutls_anon_free_client_credentials(m_gnutls_anoncred);
#endif
      m_isTLS = false;
    }
    else
#endif
    {
      close(m_fd);
    }
  }
  m_open = false;
  return 0;
}

ERROR_TYPE tcp::sendData(CPCCHAR buf, int size)
{
  if(!m_open)
    return eCorrupt;
  int sent = 0;
  m_poll.events = POLLOUT | POLLERR | POLLHUP | POLLNVAL;
  int rc;
  while(sent != size)
  {
    if(*m_exitCount > 1)
      return eCtrl_C;
    rc = poll(&m_poll, 1, 60000);
    if(rc == 0)
    {
      fprintf(stderr, "Server timed out on write.\n");
      error();
      return eTimeout;
    }
    if(rc < 0)
    {
      fprintf(stderr, "Poll error.\n");
      error();
      return eSocket;
    }
#ifdef USE_SSL
    if(m_isTLS)
    {
#ifdef USE_OPENSSL
      rc = SSL_write(m_ssl, &buf[sent], size - sent);
#else
      rc = gnutls_record_send(*m_gnutls_session, &buf[sent], size - sent);
#endif
    }
    else
#endif
    {
      rc = write(m_fd, &buf[sent], size - sent);
    }
    if(rc < 1)
    {
      fprintf(stderr, "Can't write to socket.\n");
      error();
      return eSocket;
    }
    if(m_debug)
      m_debug->Write(buf, rc);
    sent += rc;
  }
  sentData(size);
  return eNoError;
}

// fgets() doesn't
int tcp::readLine(char *buf, int bufSize)
{
  if(!m_open)
    return eCorrupt;
  int ind = 0;
  if(m_start < m_end)
  {
    do
    {
      buf[ind] = m_buf[m_start];
      ind++;
      m_start++;
    }
    while(m_start < m_end && m_buf[m_start - 1] != '\n' && ind < bufSize);
  }
  if(ind == bufSize || (ind > 0 && buf[ind - 1] == '\n') )
  {
    if(ind < bufSize)
      buf[ind] = '\0';
    receivedData(ind);
    if(m_debug)
      m_debug->Write(buf, ind);
    return ind;
  }
  // buffer is empty
  m_start = 0;
  m_end = 0;

  time_t now = time(NULL);
  m_poll.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
  while(1)
  {
    if(*m_exitCount > 1)
      return 3;
    int timeout = 60 - (time(NULL) - now);
    int rc;
    if(timeout < 0 || (rc = poll(&m_poll, 1, timeout * 1000)) == 0)
    {
      fprintf(stderr, "Server timed out on read.\n");
      error();
      return eSocket;
    }
    if(rc < 0)
    {
      if(errno == EINTR)
        continue;
      fprintf(stderr, "Poll error.\n");
      error();
      return eCorrupt;
    }
#ifdef USE_SSL
    if(m_isTLS)
    {
#ifdef USE_OPENSSL
      rc = SSL_read(m_ssl, m_buf, sizeof(m_buf));
#else
      rc = gnutls_record_recv(*m_gnutls_session, m_buf, sizeof(m_buf));
#endif
    }
    else
#endif
    {
      rc = read(m_fd, m_buf, sizeof(m_buf));
    }
    if(rc < 0)
    {
      error();
      return eSocket;
    }
    m_end = rc;
    do
    {
      buf[ind] = m_buf[m_start];
      ind++;
      m_start++;
    } while(m_start < m_end && m_buf[m_start - 1] != '\n' && ind < bufSize);

    if(ind == bufSize || (ind > 0 && buf[ind - 1] == '\n') )
    {
      if(ind < bufSize)
        buf[ind] = '\0';
      receivedData(ind);
      if(m_debug)
        m_debug->Write(buf, ind);
      return ind;
    }
    if(m_start == m_end)
    {
      m_start = 0;
      m_end = 0;
    }
  }
  return 0;
}

ERROR_TYPE tcp::sendCommandString(const string &str, bool important)
{
  return sendCommandData(str.c_str(), str.size(), important);
}

ERROR_TYPE tcp::sendCommandData(const char *buf, int size, bool important)
{
  ERROR_TYPE rc = sendData(buf, size);
  if(rc)
    return rc;
  return readCommandResp(important);
}

void tcp::endIt()
{
  if(m_open)
    close(m_fd);
  m_open = false;
}

int tcp::doAllWork(int rate)
{
  double workCount = 0.0;
  char data[2048];
  time_t lastTime = time(NULL) - RESULTS_LAG;
  int toSend;
  for(unsigned int i = 0; i < sizeof(data); i++)
    data[i] = 1;
 
  while(1)
  {
    int rc;
    if(rate)
    {
      time_t newTime = time(NULL);
      workCount += double(newTime - lastTime) / 60.0 * double(rate);
      toSend = int(workCount);
      if(toSend > int(sizeof(data)) )
      {
        toSend = sizeof(data);
        workCount = 0.0;
      }
      else
      {
        workCount -= double(toSend);
      }
      lastTime = newTime;
    }
    else
      toSend = sizeof(data);
    // NB if data can't be written then it is discarded.
    // Buffer size will be at least 1024 bytes, if worker threads aren't
    // keeping up then we don't really want more than that in the queue.
    if(toSend)
    {
      rc = WriteWork(data, toSend, 5);
      if(rc < 0)
        return -rc;
    }
 
    if(*m_exitCount > 1)
      return -1;
    rc = pollRead();
    if(rc)
      return rc;
  }
  return 0;
}

