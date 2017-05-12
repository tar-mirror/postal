#ifndef RESULTS_H
#define RESULTS_H

#include <time.h>
#include "mutex.h"

class results
{
public:
  results();
  ~results();

  void error();
  void dataBytes(int bytes);
  void message();
  void connection();

  void pollPrint(bool mustPrint = false);

private:
  void m_pollPrint(bool mustPrint = false);

  int m_msgs;
  int m_connections;
  int m_bytes;
  int m_errors;
  time_t m_timeLastAction;
  Mutex m_mut;
};

#endif
