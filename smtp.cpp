#include "smtp.h"
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "userlist.h"
#include "logit.h"
#include "results.h"

smtpData::smtpData(const char *app_name)
 : m_quit("QUIT\r\n")
 , m_randomLetters("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890 `~!@#$%^&*()-_=+[]{};:'\"|/?<>,")
 , m_randomLen(strlen(m_randomLetters))
 , m_postalMsg("X-Postal: " VER_STR " - the mad postman.\r\n"
               "X-Postal: http://www.coker.com.au/postal/\r\n"
               "X-Postal: This is not a real email.\r\n")
 , m_dnsLock(true)
 , m_timeLastAction(time(NULL))
{
  setRand(0);
}

const string * const smtpData::getMailName(struct sockaddr_in &in)
{
  Lock l(m_dnsLock);
  unsigned long ip = in.sin_addr.s_addr;
  string *name = m_names[ip];
  if(name != NULL)
    return name;
  struct hostent *h;
  h = gethostbyaddr((char *)&(in.sin_addr), sizeof(in.sin_addr), AF_INET);
  if(!h)
  {
    name = new string(inet_ntoa(in.sin_addr));
  }
  else
  {
    name = new string(h->h_name);
  }
  m_names[ip] = name;
  return name;
}

smtpData::~smtpData()
{
}

void smtpData::setRand(int frequency)
{
  if(time(NULL) - m_timeLastAction < frequency)
    return;

  for(int i = 0; i < MAP_SIZE; i++)
    m_randBuf[i] = m_randomLetters[random() % m_randomLen];
  m_timeLastAction = time(NULL);
}

string smtpData::randomString(int max_len) const
{
  if(max_len > 300)
    max_len = 300;
  max_len = random() % (max_len + 1);
  int offset = random() % (MAP_SIZE - max_len);
  string str(&m_randBuf[offset], max_len);
  str += "\r\n";
  if(!strncmp(str.c_str(), "MD5:", 4))
    str[0] = 'Z';

  return str;
}

string smtpData::date() const
{
  time_t t = time(NULL);
  t += 60 - random() % 600;
  string str = string("Date: ") + ctime(&t);
  str.insert(str.size() - 1, "\r");
  return str;
}

int smtp::action(PVOID param)
{
  while(1)
  {
    int rc = connect();
    if(rc > 1)
      return 1;
    if(rc == 0)
    {
#ifdef USE_SSL
      if(m_canTLS && (m_useTLS > rand() % 100) )
      {
        rc = sendCommandString("STARTTLS\r\n");
        if(!rc)
          rc = connectTLS();
        if(!rc)
          rc = sendCommandString(m_helo);
        if(rc > 1)
          return rc;
        m_res->ssl();
      }
#endif
      int msgs;
      if(m_msgsPerConnection == 0)
        msgs = -1;
      else if(m_msgsPerConnection < 0)
        msgs = 0;
      else
        msgs = random() % m_msgsPerConnection + 1;

      if(rc)
        msgs = 0;
      for(int i = 0; i != msgs; i++)
      {
        rc = sendMsg();
        if(rc > 1)
          return 1;
        if(rc)
          break;
      }
      if(!rc)
        rc = disconnect();
      if(rc > 1)
        return 1;
    }
    if(rc)
    {
      sleep(5);
    }
  }
}

smtp::smtp(const char *addr, const char *ourAddr
         , UserList &ul, int msgSize, int numMsgsPerConnection
         , int processes, Logit *log, TRISTATE netscape
#ifdef USE_SSL
         , bool ssl
#endif
          )
 : tcp(addr, 25, log
#ifdef USE_SSL
     , ssl
#endif
     , ourAddr)
 , m_ul(ul)
 , m_msgSize(msgSize)
 , m_data(new smtpData("Postal"))
 , m_msgsPerConnection(numMsgsPerConnection)
 , m_res(new results)
 , m_netscape(netscape)
{
  go(NULL, processes);
}

smtp::smtp(int threadNum, const smtp *parent)
 : tcp(threadNum, parent)
 , m_ul(parent->m_ul)
 , m_msgSize(parent->m_msgSize)
 , m_data(parent->m_data)
 , m_msgsPerConnection(parent->m_msgsPerConnection)
 , m_res(parent->m_res)
 , m_netscape(parent->m_netscape)
{
}

Fork *smtp::newThread(int threadNum)
{
  return new smtp(threadNum, this);
}

smtp::~smtp()
{
  if(getThreadNum() < 1)
    delete m_data;
}

void smtp::sentData(int bytes)
{
  m_res->dataBytes(bytes);
}

void smtp::receivedData(int bytes)
{
}

void smtp::error()
{
  m_res->error();
  tcp::disconnect();
}

int smtp::connect()
{
#ifdef USE_SSL
  m_canTLS = false;
#endif
  int rc = tcp::connect();
  if(rc)
    return rc;
  m_res->connection();
  rc = readCommandResp();
  if(rc)
    return rc;
  const string *mailName = m_data->getMailName(m_connectionSourceAddr);
  m_helo = string("ehlo ") + *mailName + "\r\n";
  rc = sendCommandString(m_helo);
  if(rc)
    return rc;
  return 0;
}

int smtp::disconnect()
{
  int rc = sendCommandString(m_data->quit());
  rc |= tcp::disconnect();
  return rc;
}

int smtp::sendMsg()
{
  int rc;
  int size = 0;
  if(m_msgSize)
    size = random() % (m_msgSize * 1024);
  m_md5.init();
  string logData;
  bool logAll = false;
  if(m_log && m_log->verbose())
    logAll = true;

  char aByte;
  rc = Read(&aByte, 1, 0);
  if(rc != 1)
    return 1;

  string from = string("<") + m_ul.randomUser() + '>';
  string to = string("<") + m_ul.randomUser() + '>';
  rc = sendCommandString(string("MAIL FROM: ") + from + "\r\n");
  if(rc)
    return rc;
  rc = sendCommandString(string("RCPT TO: ") + to + "\r\n");
  if(rc)
    return rc;
  rc = sendCommandString("DATA\r\n");
  if(rc)
    return rc;
  string subject = string("Subject: ");
  if(m_netscape == eWONT)
    subject += "N";
  else if(m_netscape == eMUST)
    subject += " ";
  subject += m_data->randomString(60);
  m_md5.addData(subject);
  string str = string("To: ") + to + "\r\n"
                + subject + m_data->date() + m_data->postalMsg()
                + "From: " + from + "\r\n\r\n";
  rc = sendString(str);
  if(rc)
    return rc;
  if(logAll)
    logData = str;

  int sent = 0;
  while(sent < size)
  {
    str = m_data->randomString(size - sent);
    m_md5.addData(str);
    rc = sendString(str);
    if(rc)
      return rc;
    if(logAll)
      logData += str;
    sent += str.size();
  }
  str = "MD5:";
  string sum(m_md5.getSum());
  str += sum + "\r\n.\r\n";
  rc = sendCommandString(str);
  if(rc)
    return rc;
  m_res->message();
  if(logAll)
  {
    logData += str;
    m_log->Write(logData);
  }
  else if(m_log)
  {
    sum += "\n";
    m_log->Write(sum);
  }
  return 0;
}

int smtp::readCommandResp(bool important)
{
  char recvBuf[1024];
  do
  {
    int rc = readLine(recvBuf, sizeof(recvBuf));
    if(rc < 0)
      return rc;
    if(recvBuf[0] != '2' && recvBuf[0] != '3')
    {
      printf("Server error:%s.\n", recvBuf);
      error();
      return 1;
    }
#ifdef USE_SSL
    if(!m_canTLS)
    {
      if(!strncmp("250-STARTTLS", recvBuf, 12)
         || !strncmp("250 STARTTLS", recvBuf, 12))
      {
        m_canTLS = true;
      }
    }
#endif
  }
  while(recvBuf[3] == '-');
  return 0;
}

int smtp::pollRead()
{
  m_data->setRand(RAND_TIME);
  m_res->pollPrint();
  return 0;
}

int smtp::WriteWork(PVOID buf, int size, int timeout)
{
  return Write(buf, size, timeout);
}
