#ifndef THREAD_H
#define THREAD_H

#include "port.h"

#include <sys/poll.h>
#include <pthread.h>

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
protected:
  // Virtual function that is called when the thread is started.
  // The parameter is the pointer that is passed first to the go() function
  virtual int action(PVOID param) = 0;

  // constructor for main thread class
  Thread();

  // constructor for children.
  Thread(int threadNum, const Thread *parent);
  virtual ~Thread();

  void go(PVOID param, int num); // creates all threads

  int getNumThreads() const { return m_numThreads; }

  // Virtual function to construct a new class.
  // the following comment has the implementation
  // return new class(threadNum, this);
  virtual Thread *newThread(int threadNum) = 0;

  // set the return value of the thread, probably not needed
  void setRetVal(int rc);

protected:
  int getThreadNum() const { return m_threadNum; }
  int Read(PVOID buf, int size, int timeout = 60);
  int Write(PVOID buf, int size, int timeout = 60);

protected:
  int m_read;
  int m_write;
private:

  int m_threadNum;

  pollfd m_readPoll;
  pollfd m_writePoll;
  int m_parentRead;
  int m_parentWrite;
  int m_childRead;
  int m_childWrite;
  int m_numThreads;
  int *m_retVal;
  pthread_t *m_thread_info;

  Thread(const Thread &f);
  Thread & operator =(const Thread &f);


  friend PVOID thread_func(PVOID param);
};

#endif

