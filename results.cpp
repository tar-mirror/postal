#include <stdio.h>
#include "results.h"
#include "postal.h"

results::results()
 : m_mut(true)
{
  m_timeLastAction = time(NULL);
  m_msgs = 0;
  m_connections = 0;
  m_bytes = 0;
  m_errors = 0;
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


void results::pollPrint(bool mustPrint)
{
  Lock l(m_mut);
  m_pollPrint();
}

void results::m_pollPrint(bool mustPrint)
{
  time_t now = time(NULL);
  if(mustPrint || (now - m_timeLastAction >= 60) )
  {
    tm *t = localtime(&now);
    printf("%02d:%02d,%d,%d,%d,%d\n", t->tm_hour, t->tm_min, m_msgs
                                    , m_bytes / 1024, m_errors
                                    , m_connections);
    fflush(NULL);
    m_msgs = 0;
    m_connections = 0;
    m_bytes = m_bytes % 1024;
    m_errors = 0;
    m_timeLastAction += 60;
  }
}
