#ifndef RESULTS_H
#define RESULTS_H

#include <time.h>
#include "mutex.h"

class results
{
public:
  results();
  virtual ~results();

  void error();
  void dataBytes(int bytes);
  void message();
  void connection();
#ifdef USE_SSL
  void connect_ssl();
#endif

  void print();

protected:
  virtual void childPrint();
  int m_connections;
#ifdef USE_SSL
  int m_ssl_connections;
#endif

  void m_print();

  Mutex m_mut;
private:

  int m_msgs;
  int m_bytes;
  int m_errors;
};

#endif
