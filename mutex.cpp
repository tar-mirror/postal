#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "postal.h"
#include "mutex.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

Mutex::Mutex(bool
#ifdef LINUX_PTHREAD
 fastMutex
#endif
 )
{
  int rc;
#ifdef LINUX_PTHREAD
  if(!fastMutex)
  {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    if(!rc)
      rc = pthread_mutex_init(&m_mut, &attr);
    pthread_mutexattr_destroy(&attr);
  }
  else
#endif
  {
    rc = pthread_mutex_init(&m_mut, NULL);
  }
  if(rc)
  {
    fprintf(stderr, "Can't create mutex.\n");
    exit(1);
  }
}

Mutex::~Mutex()
{
  pthread_mutex_destroy(&m_mut);
}

int Mutex::get_mutex(bool block)
{
  if(block)
  {
    if(pthread_mutex_lock(&m_mut))
      return -1;
  }
  else
  {
    if(pthread_mutex_trylock(&m_mut))
      return -1;
  }
  return 0;
}

int Mutex::put_mutex()
{
  if(pthread_mutex_unlock(&m_mut))
    return -1;
  return 0;
}

Lock::Lock(Mutex &mut)
 : m_mut(mut)
{
  if(m_mut.get_mutex())
  {
    perror("");
    fprintf(stderr, "Can't lock mutex.\n");
    exit(1);
  }
}

Lock::~Lock()
{
  if(m_mut.put_mutex())
  {
    fprintf(stderr, "Can't unlock mutex.\n");
    exit(1);
  }
}
