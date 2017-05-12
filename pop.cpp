#include "pop.h"
#include <unistd.h>
#include "userlist.h"
#include "logit.h"
#include "results.h"
#include "mutex.h"

int pop::action(PVOID param)
{
  bool logAll = false;
  if(m_log && m_log->verbose())
    logAll = true;
  while(1)
  {
    string user, pass;
    getUser(user, pass);
    int rc = connect(user, pass);
    if(rc == 0)
    {
      int msgs = 0;
      if(m_msgsPerConnection != 0)
        msgs = list();
      if(m_msgsPerConnection > 0 && msgs > m_msgsPerConnection)
        msgs = m_msgsPerConnection;
      if(msgs > 0)
      {
        for(int i = 1; i <= msgs && !rc; i++)
        {
          rc = getMsg(i, user, logAll);
        }
      }
      // if msgs < 0 means we already had a serious error
      if(!rc && msgs >= 0)
      {
        rc = disconnect();
      }
    }
    if(rc)
      sleep(10);
  }
}

pop::pop(const char *addr, const char *ourAddr, UserList &ul, int processes
       , int msgsPerConnection, Logit *log, bool ssl)
 : tcp(addr, 110, log, ssl, ourAddr)
 , m_ul(ul)
 , m_maxNameLen(ul.maxNameLen() + 1)
 , m_namesBuf(new char[m_maxNameLen * processes])
 , m_sem(new Mutex(true) )
 , m_res(new results)
 , m_msgsPerConnection(msgsPerConnection)
{
  go(NULL, processes);
}

pop::pop(int threadNum, const pop *parent)
 : tcp(threadNum, parent)
 , m_ul(parent->m_ul)
 , m_maxNameLen(parent->m_maxNameLen)
 , m_namesBuf(parent->m_namesBuf)
 , m_sem(parent->m_sem)
 , m_res(parent->m_res)
 , m_msgsPerConnection(parent->m_msgsPerConnection)
{
}

Fork *pop::newThread(int threadNum)
{
  return new pop(threadNum, this);
}

pop::~pop()
{
  if(getThreadNum() < 1)
  {
    delete m_namesBuf;
    delete m_sem;
  }
}

void pop::sentData(int bytes)
{
}

void pop::receivedData(int bytes)
{
  m_res->dataBytes(bytes);
}

void pop::error()
{
  m_res->error();
  tcp::disconnect();
}

int pop::readCommandResp()
{
  char recvBuf[1024];
  int rc = readLine(recvBuf, sizeof(recvBuf));
  if(rc < 0)
    return rc;
  if(recvBuf[0] != '+')
  {
    strtok(recvBuf, "\r\n");
    printf("Server error:%s.\n", recvBuf);
    error();
    return 1;
  }
  return 0;
}

int pop::connect(const string &user, const string &pass)
{
  char aByte;
  int rc = Read(&aByte, 1, 0);
  if(rc != 1)
    return 1;
  rc = tcp::connect();
  if(rc)
    return rc;
  m_res->connection();
  rc = readCommandResp();
  if(rc)
    return rc;
  string u("user ");
  u += user;
  u += "\r\n";
  rc = sendCommandString(u);
  if(rc)
    return rc;
  string p("pass ");
  p += pass;
  p += "\r\n";
  rc = sendCommandString(p);
  if(rc)
    return rc;
  return 0;
}

int pop::disconnect()
{
  int rc = sendCommandData("quit\r\n", 6);
// Comment the next line to make it that if the number of threads equals the
// number of accounts then each thread always does the same account.
// This makes things easy to debug and has no real down-side, I don't do this
// by default because it lightens the load a little on mail servers.
  memset(&m_namesBuf[m_threadNum * m_maxNameLen], 0, m_maxNameLen);
// NB The sendCommandData will wait for a +OK or -ERR response, in either
// case the server is (or should be ;) ready for another connection.
  if(rc)
    return rc;
  return tcp::disconnect();
}

int pop::getMsg(int num, const string &user, bool log)
{
  char command[14];
  sprintf(command, "retr %d\r\n", num);
  int rc;
  rc = sendCommandData(command, strlen(command));
  if(rc)
    return rc + 1;

  char buf[1024];

  bool postal = false;
  bool gotSubject = false;
  enum STATE { eHeader, eBody, eEnd } state = eHeader;
  m_md5.init();
  bool md5Error = false;
  string logData;
  do
  {
    rc = readLine(buf, sizeof(buf));
    if(rc < 0)
      return rc;
    if(log)
      logData += string(buf);
    switch(state)
    {
    case eHeader:
      if(!gotSubject && !strncmp(buf, "Subject: ", 9))
      {
        gotSubject = true;
        m_md5.addData(buf, strlen(buf));
      }
      else if(!postal && !strncmp(buf, "X-Postal: ", 10))
      {
        postal = true;
      }
      else if(!strcmp(buf, "\r\n"))
      {
        state = eBody;
      }
    break;
    case eBody:
      if(postal)
      {
        if(!strncmp(buf, "MD5:", 4))
        {
          string sum(m_md5.getSum());
          strtok(buf, "\r");
          if(sum != &buf[4])
          {
            if(log || !m_log)
            {
              string errStr("MD5 mis-match, calculated:");
              errStr += sum + ", expected " + &buf[4] + "!\nAccount name "
                      + user + "\n";
              if(m_log)
                m_log->Write(errStr);
              fprintf(stderr, "%s", errStr.c_str());
            }
            md5Error = true;
          }
          else
          {
            if(m_log)
            {
              sum += "\n";
              m_log->Write(sum);
            } 
          }
          state = eEnd;
        }
        else
        {
          m_md5.addData(buf, strlen(buf));
        }
      }
    break;
    case eEnd:
      if(strcmp(buf, ".\r\n"))
      {
        fprintf(stderr, "End of message data but not end of POP data.\n");
        md5Error = true;
      }
    break;
    }
  }
  while(strcmp(buf, ".\r\n"));

  if(log)
    m_log->Write(logData);

  // if we didn't log this, but we can do logging and there was an error, then
  // retrieve the message again with logging so we know what went wrong.
  if(!log && m_log && md5Error)
    return getMsg(num, user, true);
  if(md5Error)
    m_res->error();

  sprintf(command, "dele %d\r\n", num);
  rc = sendCommandData(command, strlen(command));
  if(rc)
    return rc;
  m_res->message();
  return 0;
}

int pop::list()
{
  int rc = sendCommandData("list\r\n", 6);
  if(rc)
    return rc;
  // The number of messages is the number of lines of response -1 because
  // we get one line of ".\r\n" at the end.
  int i = -1;
  char buf[1024];
  do
  {
    rc = readLine(buf, sizeof(buf));
    if(rc < 0)
      return rc;
    i++;
  }
  while(strcmp(buf, ".\r\n"));
  return i;
}

bool pop::checkUser(const char *user)
{
  int num = getNumThreads();
  Lock l(*m_sem);
  if(num == 1)
    return true;
  for(int i = 0; i < num; i++)
  {
    if(i != m_threadNum && !strcmp(user, &m_namesBuf[i * m_maxNameLen]))
    {
      return false;
    }
  }
  strcpy(&m_namesBuf[m_threadNum * m_maxNameLen], user);
  return true;
}

void pop::getUser(string &user, string &pass)
{
  user = m_ul.randomUser();
  while(!checkUser(user.c_str()) )
  {
    user = m_ul.sequentialUser();
  }
  pass = m_ul.password();
}

int pop::pollRead()
{
  m_res->pollPrint();
  return 0;
}

int pop::WriteWork(PVOID buf, int size, int timeout)
{
  return Write(buf, size, timeout);
}
