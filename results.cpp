#include <stdio.h>
#include "postal.h"
#include "results.h"

results::results()
 : m_connections(0)
#ifdef USE_SSL
 , m_ssl_connections(0)
#endif
 , m_mut(true)
 , m_msgs(0)
 , m_bytes(0)
 , m_errors(0)
{
}

results::~results()
{
  m_print();
}

void results::error()
{
  Lock l(m_mut);
  m_errors++;
}

void results::dataBytes(int bytes)
{
  Lock l(m_mut);
  m_bytes += bytes;
}

void results::message()
{
  Lock l(m_mut);
  m_msgs++;
}

void results::connection()
{
  Lock l(m_mut);
  m_connections++;
}

#ifdef USE_SSL
void results::ssl()
{
  Lock l(m_mut);
  m_ssl_connections++;
}
#endif

void results::print()
{
  Lock l(m_mut);
  m_print();
}

void results::childPrint()
{
}

void results::m_print()
{
  time_t now = time(NULL);
    tm *t = localtime(&now);
    printf("%02d:%02d,%d,%d,%d", t->tm_hour, t->tm_min, m_msgs
                               , m_bytes / 1024, m_errors);
#ifdef USE_SSL
    printf(",%d,%d", m_connections, m_ssl_connections);
    m_ssl_connections = 0;
#else
    printf(",%d", m_connections);
#endif
    childPrint();
    printf("\n");
    fflush(NULL);
    m_msgs = 0;
    m_connections = 0;
    m_bytes = m_bytes % 1024;
    m_errors = 0;
}
