#include "client.h"
#include <cstdlib>
#include <unistd.h>
#include "userlist.h"
#include "logit.h"
#include "results.h"
#include "mutex.h"

class clientResults : public results
{
public:
  clientResults();

  void imap_connection();

private:
  virtual void childPrint();
  int m_imap_connections;
};

clientResults::clientResults()
 : results()
 , m_imap_connections(0)
{
}

void clientResults::childPrint()
{
  printf(",%d", m_imap_connections);
  m_imap_connections = 0;
}

void clientResults::imap_connection()
{
  Lock l(m_mut);
  m_connections++;
  m_imap_connections++;
}

Thread *client::newThread(int threadNum)
{
  return new client(threadNum, this);
}

int client::action(PVOID)
{
  bool logAll = false;
  if(m_log && m_log->verbose())
    logAll = true;
  while(1)
  {
    string user, pass;
    getUser(user, pass);
    if(CHECK_PERCENT(m_useIMAP))
      m_isIMAP = true;
    else
      m_isIMAP = false;
    int rc = Connect(user, pass);
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
          if(CHECK_PERCENT(m_downloadPercent))
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

client::client(int *exitCount, const char *addr, const char *ourAddr
       , UserList &ul, int processes, int msgsPerConnection, Logit *log
#ifdef USE_SSL
       , int ssl
#endif
       , TRISTATE qmail_pop, int imap, int downloadPercent, int deletePercent
       , Logit *debug)
 : tcp(exitCount, addr, 110, log
#ifdef USE_SSL
     , ssl
#endif
     , ourAddr, debug)
 , m_ul(ul)
 , m_maxNameLen(ul.maxNameLen() + 1)
 , m_namesBuf(new char[m_maxNameLen * (processes + 1)])
 , m_sem(new Mutex(true) )
 , m_res(new clientResults)
 , m_msgsPerConnection(msgsPerConnection)
 , m_useIMAP(imap)
 , m_isIMAP(false)
 , m_imapID(0)
 , m_qmail_pop(qmail_pop)
 , m_downloadPercent(downloadPercent)
 , m_deletePercent(deletePercent)
{
  go(NULL, processes);
}

client::client(int threadNum, const client *parent)
 : tcp(threadNum, parent)
 , m_ul(parent->m_ul)
 , m_maxNameLen(parent->m_maxNameLen)
 , m_namesBuf(parent->m_namesBuf)
 , m_sem(parent->m_sem)
 , m_res(parent->m_res)
 , m_msgsPerConnection(parent->m_msgsPerConnection)
 , m_useIMAP(parent->m_useIMAP)
 , m_isIMAP(false)
 , m_imapID(0)
 , m_qmail_pop(parent->m_qmail_pop)
 , m_downloadPercent(parent->m_downloadPercent)
 , m_deletePercent(parent->m_deletePercent)
{
}

client::~client()
{
  if(getThreadNum() < 1)
  {
    delete m_namesBuf;
    delete m_sem;
  }
}

void client::sentData(int)
{
}

void client::receivedData(int bytes)
{
  m_res->dataBytes(bytes);
}

void client::error()
{
  m_res->error();
  tcp::disconnect();
}

ERROR_TYPE client::readCommandResp(bool important)
{
  char recvBuf[1024];
  int rc;
  if(m_isIMAP)
  {
    do
    {
      rc = readLine(recvBuf, sizeof(recvBuf));
      if(rc < 0)
        return ERROR_TYPE(rc);
    }
    while(recvBuf[0] == '*');

    if(strncmp(recvBuf, m_imapIDtxt, strlen(m_imapIDtxt))
       || strncmp(&recvBuf[strlen(m_imapIDtxt)], "OK", 2) )
    {
      if(important)
      {
        strtok(recvBuf, "\r\n");
        fprintf(stderr, "Server error:%s.\n", recvBuf);
        error();
      }
      return eServer;
    }
  }
  else
  {
    rc = readLine(recvBuf, sizeof(recvBuf));
    if(rc < 0)
      return ERROR_TYPE(rc);
    if(recvBuf[0] != '+')
    {
      if(important)
      {
        strtok(recvBuf, "\r\n");
        fprintf(stderr, "Server error:%s.\n", recvBuf);
        error();
      }
      return eServer;
    }
  }
  return eNoError;
}

int client::Connect(const string &user, const string &pass)
{
  char aByte;
  int rc = Read(&aByte, 1, 0);
  if(rc != 1)
    return eCorrupt;
  if(m_isIMAP)
    return connectIMAP(user, pass);
  return connectPOP(user, pass);
}

int client::connectPOP(const string &user, const string &pass)
{
  int rc = tcp::Connect();
  if(rc)
    return rc;
  m_res->connection();
  rc = readCommandResp();
  if(rc)
  {
    fprintf(stderr, "Can't establish connection.\n");
    disconnect();
    return rc;
  }
  // not supporting CAPA is OK.
  rc = sendCommandString("CAPA\r\n", false);
  if(rc > 1)
    return rc;
  if(rc == 0) // successful
  {
    char buf[1024];
    // read all lines of the capa field until the ".\r\n"
    do
    {
      rc = readLine(buf, sizeof(buf) - 1);
      if(rc < 0)
        return 2;
      buf[sizeof(buf) - 1] = '\0';
      strtok(buf, "\r\n");
#ifdef USE_SSL
      if(!strcasecmp(buf, "STLS"))
        m_canTLS = true;
#endif
    } while(strcmp(".", buf));
  }
  string u("user ");
  u += user;
  u += "\r\n";
  rc = sendCommandString(u);
//printf("%s\n", u.c_str());
  if(rc)
    return rc;
  string p("pass ");
  p += pass;
  p += "\r\n";
  rc = sendCommandString(p);
//printf("%s\n", p.c_str());
  if(rc)
    return rc;
  return 0;
}

int client::connectIMAP(const string &user, const string &pass)
{
  m_imapID = 0;
  int rc = tcp::Connect(143);
  if(rc)
    return rc;
  m_res->connection();
  rc = sendCommandString("C CAPABILITY\r\n");
// check for sub-string "STARTTLS"
  if(rc)
    return rc;
  string u("C LOGIN ");
  u += user + " " + pass;
  u += "\r\n";
  rc = sendCommandString(u);
  if(rc)
    return rc;
  return 0;
}

int client::disconnect()
{
  int rc;
  if(m_isIMAP)
    rc = sendCommandData("C LOGOUT\r\n", 6);
  else
    rc = sendCommandData("quit\r\n", 6);

  if(!rc)
    rc = tcp::disconnect();
// Comment the next line to make it that if the number of threads equals the
// number of accounts then each thread always does the same account.
// This makes things easy to debug and has no real down-side, I don't do this
// by default because it lightens the load a little on mail servers.
  memset(&m_namesBuf[getThreadNum() * m_maxNameLen], 0, m_maxNameLen);
// NB The sendCommandData will wait for a +OK or -ERR response, in either
// case the server is (or should be ;) ready for another connection.
  return rc;
}

int client::getMsg(int num, const string &user, bool log)
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
        if(strcmp(buf, "\r\n") && m_qmail_pop != eWONT)
        {
          fprintf(stderr, "End of message data but not end of POP data.\n");
          md5Error = true;
        }
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

  if(CHECK_PERCENT(m_deletePercent))
  {
    sprintf(command, "dele %d\r\n", num);
    rc = sendCommandData(command, strlen(command));
  }
  if(rc)
    return rc;
  m_res->message();
  return 0;
}

int client::list()
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

bool client::checkUser(const char *user)
{
  int num = getNumThreads();
  if(num == 1)
    return true;
  for(int i = 1; i <= num; i++)
  {
    if(i != getThreadNum() && !strcmp(user, &m_namesBuf[i * m_maxNameLen]))
    {
      return false;
    }
  }
  strcpy(&m_namesBuf[getThreadNum() * m_maxNameLen], user);
  return true;
}

void client::getUser(string &user, string &pass)
{
  Lock l(*m_sem);
  user = m_ul.randomUser();
  while(!checkUser(user.c_str()) )
  {
    user = m_ul.sequentialUser();
  }
  pass = m_ul.password();
}

int client::pollRead()
{
  return 0;
}

int client::WriteWork(PVOID buf, int size, int timeout)
{
  return Write(buf, size, timeout);
}

ERROR_TYPE client::sendCommandString(const string &s, bool important)
{
  if(m_isIMAP)
  {
    m_imapID++;
    sprintf(m_imapIDtxt, "%d ", m_imapID);
    string command(m_imapIDtxt);
    command += s;
    return sendCommandData(command.c_str(), command.size(), important);
  }
  return sendCommandData(s.c_str(), s.size(), important);
}
