
#ifdef OS2
#define INCL_DOSPROCESS
#else
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#endif

#include "forkit.h"

#include <stdio.h>


Fork::Fork()
{
  m_read = -1;
  m_write = -1;
  m_threadNum = -1;
  m_numThreads = 0;

  m_parentRead = -1;
  m_parentWrite = -1;
  m_childRead = -1;
  m_childWrite = -1;
  m_retVal = NULL;
}

Fork::Fork(int threadNum, const Fork *parent)
{
  m_threadNum = threadNum;
  m_read = parent->m_childRead;
  m_write = parent->m_childWrite;
  m_numThreads = parent->m_numThreads;

  m_parentRead = -1;
  m_parentWrite = -1;
  m_childRead = -1;
  m_childWrite = -1;
  m_retVal = &parent->m_retVal[threadNum];
}

Fork::~Fork()
{
  if(m_parentRead != -1)
  {
    close(m_parentRead);
    close(m_parentWrite);
    close(m_childRead);
    close(m_childWrite);
    delete m_retVal;
  }
}

// for the benefit of this function and the new Fork class it may create
// the Fork class must do nothing of note in it's constructor or it's
// go() member function.
#ifdef OS2
VOID APIENTRY thread_func(ULONG param)
#else
PVOID thread_func(PVOID param)
#endif
{
  THREAD_DATA *td = (THREAD_DATA *)param;
  Fork *thread = td->f->newThread(td->threadNum);
  thread->setRetVal(thread->action(td->param));
  delete thread;
  delete td;
  return NULL;
}

void Fork::go(PVOID param, int num)
{
  m_numThreads += num;
  FILE_TYPE control[2];
  FILE_TYPE feedback[2];
  if (pipe(feedback) || pipe(control))
  {
    fprintf(stderr, "Can't open pipes.\n");
    exit(1);
  }
  m_parentRead = feedback[0];
  m_parentWrite = control[1];
  m_childRead = control[0];
  m_childWrite = feedback[1];
  m_read = m_parentRead;
  m_write = m_parentWrite;
#ifndef OS2
  m_readPoll.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
  m_writePoll.events = POLLOUT | POLLERR | POLLHUP | POLLNVAL;
  m_readPoll.fd = m_parentRead;
  m_writePoll.fd = m_parentWrite;
  pthread_t *thread_info = new pthread_t[num];
  pthread_attr_t attr;
  if(pthread_attr_init(&attr))
    fprintf(stderr, "Can't init thread attributes.\n");
  if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
    fprintf(stderr, "Can't set thread attributes.\n");
#endif

  m_retVal = new int[num + 1];
  for(int i = 1; i <= num; i++)
  {
    m_retVal[i] = -1;
    THREAD_DATA *td = new THREAD_DATA;
    td->f = this;
    td->param = param;
    td->threadNum = i;
#ifdef OS2
    // yes I know I am casting a pointer to an unsigned long
    // it's the way you're supposed to do things in OS/2
    TID id = 0;
    if(DosCreateThread(&id, thread_func, ULONG(td), CREATE_READY, 32*1024))
    {
      fprintf(stderr, "Can't create a thread.\n");
      exit(1);
    }
#else
    int p = pthread_create(&thread_info[i], &attr, thread_func, PVOID(td));
    if(p)
    {
      fprintf(stderr, "Can't create a thread.\n");
      exit(1);
    }
#endif
  }
#ifndef OS2
  if(pthread_attr_destroy(&attr))
    fprintf(stderr, "Can't destroy thread attributes.\n");
  m_readPoll.fd = m_read;
  m_writePoll.fd = m_write;
#endif
}

void Fork::setRetVal(int rc)
{
  *m_retVal = rc;
}

int Fork::Read(PVOID buf, int size, int timeout)
{
#ifndef OS2
  if(timeout)
  {
    int rc = poll(&m_readPoll, 1, timeout * 1000);
    if(rc < 0)
    {
      fprintf(stderr, "Can't poll read ITC.\n");
      return -1;
    }
    if(!rc)
      return 0;
  }
#endif
#ifdef OS2
  unsigned long actual;
  int rc = DosRead(m_read, buf, size, &actual);
  if(rc || actual != size)
#else
  if(size != read(m_read, buf, size) )
#endif
  {
    fprintf(stderr, "Can't read data from ITC pipe.\n");
    return -1;
  }
  return size;
}

int Fork::Write(PVOID buf, int size, int timeout)
{
#ifndef OS2
  if(timeout)
  {
    int rc = poll(&m_writePoll, 1, timeout * 1000);
    if(rc < 0)
    {
perror("");
      fprintf(stderr, "Can't poll write ITC.\n");
      return -1;
    }
    if(!rc)
      return 0;
  }
#endif
#ifdef OS2
  unsigned long actual;
  int rc = DosWrite(m_write, buf, size, &actual);
  if(rc || actual != size)
#else
  if(size != write(m_write, buf, size))
#endif
  {
    fprintf(stderr, "Can't write data to ITC pipe.\n");
    return -1;
  }
  return size;
}
