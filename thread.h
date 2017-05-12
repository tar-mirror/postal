#ifndef THREAD_H
#define THREAD_H

#ifndef OS2
#include <sys/poll.h>
#endif

#include "port.h"

class Thread;

typedef void *PVOID;

typedef struct
{
  Thread *f;
  PVOID param;
  int threadNum;
} THREAD_DATA;

class Thread
{
public:
  virtual int action(PVOID param) = 0;

  Thread();
  Thread(int threadNum, const Thread *parent);
  virtual ~Thread();
  void go(PVOID param, int num); // creates all threads

  int getNumThreads() const { return m_numThreads; }

  virtual Thread *newThread(int threadNum) = 0;

  void setRetVal(int rc);

protected:
  int getThreadNum() const { return m_threadNum; }
  int Read(PVOID buf, int size, int timeout = 60);
  int Write(PVOID buf, int size, int timeout = 60);


private:

  int m_threadNum;

#ifndef OS2
  pollfd m_readPoll;
  pollfd m_writePoll;
#endif
  FILE_TYPE m_read;
  FILE_TYPE m_write;
  FILE_TYPE m_parentRead;
  FILE_TYPE m_parentWrite;
  FILE_TYPE m_childRead;
  FILE_TYPE m_childWrite;
  int m_numThreads;
  int *m_retVal;
};

#endif

