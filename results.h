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
  void ssl();

  void pollPrint(bool mustPrint = false);

protected:
  virtual void childPrint();
  int m_connections;
  int m_ssl_connections;

  void m_pollPrint(bool mustPrint = false);

  Mutex m_mut;
private:

  int m_msgs;
  int m_bytes;
  int m_errors;
  time_t m_timeLastAction;
};

#endif
