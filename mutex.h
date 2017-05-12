#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>

class Mutex
{
public:

  Mutex(bool fastMutex);
  ~Mutex();

  int get_mutex(bool block = true);
  int put_mutex();

private:
  pthread_mutex_t m_mut;
};

class Lock
{
public:
  Lock(Mutex &mut);

  ~Lock();

private:
  Mutex &m_mut;
};

#endif

