#define TCP_BODY

#include "basictcp.h"

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include "postal.h"
#include "userlist.h"
#include "address.h"
#include "logit.h"
#include "results.h"

#ifdef USE_GNUTLS
int base_tcp::m_init_dh_params = 0;
gnutls_dh_params_t base_tcp::m_dh_params;
#endif

base_tcp::base_tcp(int fd, Logit *log, Logit *debug, results *res
#ifdef USE_SSL
       , int ssl
#endif
      ) :
#ifdef USE_SSL
   m_canTLS(false),
   m_useTLS(ssl),
#endif
   m_sock(fd)
 , m_start(0)
 , m_end(0)
 , m_open(true)
 , m_log(log)
 , m_debug(debug)
 , m_res(res)
#ifdef USE_SSL
#ifdef USE_OPENSSL
 , m_sslMeth(NULL)
 , m_sslCtx(NULL)
 , m_ssl(NULL)
#else
 , m_gnutls_session(NULL)
#endif
 , m_isTLS(false)
#endif
{
  m_poll.fd = m_sock;
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

base_tcp::~base_tcp()
{
}

#ifdef USE_SSL

#ifdef USE_GNUTLS
#define DH_BITS 1024

void base_tcp::m_initialize_tls_session()
{
  const int kx_prio[] = { GNUTLS_KX_ANON_DH, 0 };

  gnutls_init(&m_gnutls_session, GNUTLS_SERVER);

  /* avoid calling all the priority functions, since the defaults
   * are adequate.
   */
  gnutls_set_default_priority(m_gnutls_session);
  gnutls_kx_set_priority(m_gnutls_session, kx_prio);

  gnutls_credentials_set(m_gnutls_session, GNUTLS_CRD_ANON, m_anoncred);

  gnutls_dh_set_prime_bits(m_gnutls_session, DH_BITS);
}

void base_tcp::m_generate_dh_params()
{
  /* Generate Diffie Hellman parameters - for use with DHE
   * kx algorithms. These should be discarded and regenerated
   * once a day, once a week or once a month. Depending on the
   * security requirements.
   */
  gnutls_dh_params_init(&m_dh_params);
  gnutls_dh_params_generate2(m_dh_params, DH_BITS);
}
#endif // USE_GNUTLS

int base_tcp::ConnectTLS()
{
#ifdef USE_OPENSSL
  m_sslMeth = NULL;
  m_sslCtx = NULL;
  m_ssl = NULL;
  m_sslMeth = SSLv2_client_method();
  if(m_sslMeth == NULL)
  {
    fprintf(stderr, "Can't get SSLv2_client_method.\n");
    return 2;
  }
  m_sslCtx = SSL_CTX_new(m_sslMeth);
  if(m_sslCtx == NULL)
  {
    fprintf(stderr, "Can't SSL_CTX_new\n");
    return 2;
  }
  if((m_ssl = SSL_new(m_sslCtx)) == NULL)
  {
    fprintf(stderr, "Can't SSL_new\n");
    SSL_CTX_free(m_sslCtx);
    return 2;
  }
  SSL_set_fd(m_ssl, m_sock);
  if(-1 == SSL_connect(m_ssl))
  {
    fprintf(stderr, "Can't SSL_CONNECT\n");
    SSL_free(m_ssl);
    SSL_CTX_free(m_sslCtx);
    return 1;
  }
  m_isTLS = true;

// debugging code that may be useful to have around in a commented-out state.
#if 0
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
#endif  // 0
#else

  gnutls_anon_allocate_server_credentials(&m_anoncred);
  m_initialize_tls_session();

  if(!m_init_dh_params)
  {
    m_init_dh_params = 1;
    m_generate_dh_params();
  }

  gnutls_anon_set_server_dh_params(m_anoncred, m_dh_params);

  gnutls_transport_set_ptr(m_gnutls_session, (gnutls_transport_ptr_t)m_sock);
  int rc = gnutls_handshake(m_gnutls_session);
  if(rc < 0)
  {
    gnutls_deinit(m_gnutls_session);
    return 2;
  }
  m_isTLS = 1;

  /* request client certificate if any.
   */
  gnutls_certificate_server_set_request(m_gnutls_session, GNUTLS_CERT_REQUEST);

#endif // USE_OPENSSL
  return 0;
}
#endif // USE_SSL

int base_tcp::disconnect()
{
  if(m_open)
  {
#ifdef USE_SSL
    if(m_isTLS)
    {
#ifdef USE_OPENSSL
      SSL_shutdown(m_ssl);
      close(m_sock);
      SSL_free(m_ssl);
      SSL_CTX_free(m_sslCtx);
      m_isTLS = false;
#else
#endif
    }
    else
#endif
    {
      close(m_sock);
    }
  }
  m_open = false;
  return 0;
}

ERROR_TYPE base_tcp::printf(CPCCHAR fmt, ...)
{
  va_list argp;
  va_start(argp, fmt);
  char buf[1024];
  int len = vsnprintf(buf, sizeof(buf), fmt, argp);
  if(len > (int)sizeof(buf))
    len = sizeof(buf);
  return sendData(buf, len);
}

ERROR_TYPE base_tcp::sendData(CPCCHAR buf, int size)
{
  if(!m_open)
    return eCorrupt;
  int sent = 0;
  m_poll.events = POLLOUT | POLLERR | POLLHUP;
  int rc;
  while(sent != size)
  {
    rc = poll(&m_poll, 1, 60000);
    if(rc == 0)
    {
      fprintf(stderr, "Server timed out on write.\n");
      return eTimeout;
    }
    if(rc < 0)
    {
      fprintf(stderr, "Poll error.\n");
      return eSocket;
    }
#ifdef USE_SSL
    if(m_isTLS)
    {
#ifdef USE_OPENSSL
      rc = SSL_write(m_ssl, &buf[sent], size - sent);
#else
      rc = gnutls_record_send(m_gnutls_session, &buf[sent], size - sent);
#endif
    }
    else
#endif
    {
      rc = write(m_sock, &buf[sent], size - sent);
    }
    if(rc < 1)
    {
//      fprintf(stderr, "Can't write to socket.\n");
      return eSocket;
    }
    if(m_debug)
      m_debug->Write(buf, rc);
    sent += rc;
  }
  sentData(size);
  return eNoError;
}

int base_tcp::readLine(char *buf, int bufSize, bool stripCR, int timeout)
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
    receivedData(ind);
    if(m_debug)
      m_debug->Write(buf, ind);
    if(ind < bufSize)
    {
      ind--;
      buf[ind] = '\0';
      if(stripCR && buf[ind - 1] == '\r')
      {
        ind--;
        buf[ind] = '\0';
      }
    }
    return ind;
  }
  // buffer is empty
  m_start = 0;
  m_end = 0;

  time_t now = time(NULL);
  m_poll.events = POLLIN | POLLERR | POLLHUP;
  while(1)
  {
    int tmo = timeout - (time(NULL) - now);
    int rc;
    if(tmo < 0 || (rc = poll(&m_poll, 1, tmo * 1000)) == 0)
    {
      return eTimeout;
    }
    if(rc < 0)
    {
      fprintf(stderr, "Poll error.\n");
      return eCorrupt;
    }
#ifdef USE_SSL
    if(m_isTLS)
    {
#ifdef USE_OPENSSL
      rc = SSL_read(m_ssl, m_buf, sizeof(m_buf));
#else
      rc = gnutls_record_recv(m_gnutls_session, m_buf, sizeof(m_buf));
#endif
    }
    else
#endif
    {
      rc = read(m_sock, m_buf, sizeof(m_buf));
    }
    if(rc < 0)
      return eSocket;
    m_end = rc;
    do
    {
      buf[ind] = m_buf[m_start];
      ind++;
      m_start++;
    } while(m_start < m_end && m_buf[m_start - 1] != '\n' && ind < bufSize);

    if(ind == bufSize || (ind > 0 && buf[ind - 1] == '\n') )
    {
      receivedData(ind);
      if(m_debug)
        m_debug->Write(buf, ind);
      if(ind < bufSize)
      {
        ind--;
        buf[ind] = '\0';
        if(stripCR && buf[ind - 1] == '\r')
        {
          ind--;
          buf[ind] = '\0';
        }
      }
      return ind;
    }
    if(m_start == m_end)
    {
      m_start = 0;
      m_end = 0;
    }
  }
  return 0; // never reached
}

void base_tcp::sentData(int)
{
}

void base_tcp::receivedData(int bytes)
{
  m_res->dataBytes(bytes);
}

