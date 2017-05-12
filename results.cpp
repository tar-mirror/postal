#include <stdio.h>
#include "results.h"
#include "postal.h"

results::results()
 : m_connections(0)
 , m_ssl_connections(0)
 , m_mut(true)
 , m_msgs(0)
 , m_bytes(0)
 , m_errors(0)
 , m_timeLastAction(time(NULL))
{
}

results::~results()
{
  m_pollPrint(true);
}

void results::error()
{
  Lock l(m_mut);
  m_errors++;
  m_pollPrint();
}

void results::dataBytes(int bytes)
{
  Lock l(m_mut);
  m_bytes += bytes;
  m_pollPrint();
}

void results::message()
{
  Lock l(m_mut);
  m_msgs++;
  m_pollPrint();
}

void results::connection()
{
  Lock l(m_mut);
  m_connections++;
  m_pollPrint();
}

void results::ssl()
{
  Lock l(m_mut);
  m_ssl_connections++;
  m_pollPrint();
}

void results::pollPrint()
{
  Lock l(m_mut);
  m_pollPrint();
}

void results::childPrint()
{
}

void results::m_pollPrint(bool mustPrint)
{
  time_t now = time(NULL);
  if(mustPrint || (now - m_timeLastAction >= 60) )
  {
    tm *t = localtime(&now);
    printf("%02d:%02d,%d,%d,%d", t->tm_hour, t->tm_min, m_msgs
                               , m_bytes / 1024, m_errors);
    printf(",%d,%d", m_connections, m_ssl_connections);
    childPrint();
    printf("\n");
    fflush(NULL);
    m_msgs = 0;
    m_connections = 0;
    m_ssl_connections = 0;
    m_bytes = m_bytes % 1024;
    m_errors = 0;
    m_timeLastAction += 60;
  }
}
