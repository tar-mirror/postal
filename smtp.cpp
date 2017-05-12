#include "smtp.h"
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include "userlist.h"
#include "logit.h"
#include "results.h"
#include <cstring>

smtpData::smtpData()
 : m_quit("QUIT\r\n")
 , m_randomLetters("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890 `~!@#$%^&*()-_=+[]{};:'\"|/?<>,")
 , m_randomLen(strlen(m_randomLetters))
 , m_postalMsg("\r\nX-Postal: " VER_STR " - the mad postman.\r\n"
               "X-Postal: http://www.coker.com.au/postal/\r\n"
               "X-Postal: This is not a real email.\r\n\r\n")
 , m_dnsLock(true)
 , m_timeLastAction(time(NULL))
{
  setRand(0);
}

const string *smtpData::getMailName(struct sockaddr_in &in)
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
    FILE *fp = fopen("/etc/mailname", "r");
    char buf[100];
    if(fp && fgets(buf, sizeof(buf), fp))
    {
      char *tmp = strchr(buf, '\n');
      if(tmp)
        *tmp = 0;
      name = new string(buf);
    }
    else
    { 
      name = new string(inet_ntoa(in.sin_addr));
    }
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

void smtpData::randomString(char *buf, int len) const
{
  if(len > 2)
  {
    int offset = random() % (MAP_SIZE - (len - 2));
    memcpy(buf, &m_randBuf[offset], len - 2);
  }
  strcpy(buf + len - 2, "\r\n");
}

const int max_line_len = 79;

void smtpData::randomBuf(char *buf, int len) const
{
  while(len)
  {
    int line_len = random() % max_line_len;
    if(line_len < 2)
      line_len = 2;
    if(len - line_len < 2)
      line_len = len;
    randomString(buf, line_len);
    len -= line_len;
    buf += line_len;
  }
}

// Return a random date that may be as much as 60 seconds in the future or 600 seconds in the past.
// buffer must be at least 34 bytes for "Day, dd Mon yyyy hh:mm:ss +zzzz\r\n"
void smtpData::date(char *buf) const
{
  time_t t = time(NULL);
  struct tm broken;

  t += 60 - random() % 600;

  if(!gmtime_r(&t, &broken) || !strftime(buf, 34, "%a, %d %b %Y %H:%M:%S %z\r\n", &broken))
    strcpy(buf, "Error making date");
}

const string smtpData::msgId(const char *sender, const unsigned threadNum) const
{
  char msgId_buf[256];
  const unsigned int max_sender_len = sizeof(msgId_buf) - 35;

  if(strlen(sender) > max_sender_len)
    sender += strlen(sender) - max_sender_len;
  else if(*sender == '<')
    sender++;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  snprintf(msgId_buf, sizeof(msgId_buf), "<%08X.%03X.%03X.%s\r\n", unsigned(tv.tv_sec), unsigned(tv.tv_usec % 2048), threadNum % 2048, sender);
  return string(msgId_buf);
}

Thread *smtp::newThread(int threadNum)
{
  return new smtp(threadNum, this);
}

int smtp::action(PVOID)
{
  while(1)
  {
    int rc = Connect();
    if(rc > 1)
      return 1;
    if(rc == 0)
    {
#ifdef USE_SSL
      if(m_canTLS && CHECK_PERCENT(m_useTLS) )
      {
        rc = sendCommandString("STARTTLS\r\n");
        if(!rc)
          rc = ConnectTLS();
        if(!rc)
          rc = sendCommandString(m_helo);
        if(rc > 1)
          return rc;
        m_res->connect_ssl();
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
        if(*m_exitCount)
        {
          disconnect();
          return 1;
        }
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

smtp::smtp(int *exitCount, const char *addr, const char *ourAddr, UserList &ul
         , UserList *senderList, int minMsgSize, int maxMsgSize
         , int numMsgsPerConnection
         , int processes, Logit *log, TRISTATE netscape, bool useLMTP
#ifdef USE_SSL
         , int ssl
#endif
         , unsigned short port, Logit *debug)
 : tcp(exitCount, addr, port, log
#ifdef USE_SSL
     , ssl
#endif
     , ourAddr, debug)
 , m_ul(ul)
 , m_senderList(senderList ? senderList : &ul)
 , m_minMsgSize(minMsgSize * 1024)
 , m_maxMsgSize(maxMsgSize * 1024)
 , m_data(new smtpData())
 , m_msgsPerConnection(numMsgsPerConnection)
 , m_res(new results)
 , m_netscape(netscape)
 , m_nextPrint(time(NULL)/60*60+60)
 , m_useLMTP(useLMTP)
{
  go(NULL, processes);
}

smtp::smtp(int threadNum, const smtp *parent)
 : tcp(threadNum, parent)
 , m_ul(parent->m_ul)
 , m_senderList(parent->m_senderList)
 , m_minMsgSize(parent->m_minMsgSize)
 , m_maxMsgSize(parent->m_maxMsgSize)
 , m_data(parent->m_data)
 , m_msgsPerConnection(parent->m_msgsPerConnection)
 , m_res(parent->m_res)
 , m_netscape(parent->m_netscape)
 , m_nextPrint(0)
{
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

void smtp::receivedData(int)
{
}

void smtp::error()
{
  m_res->error();
  tcp::disconnect();
}

int smtp::Connect()
{
  int rc = tcp::Connect();
  if(rc)
    return rc;
  m_res->connection();
  rc = readCommandResp();
  if(rc)
    return rc;
  const string *mailName = m_data->getMailName(m_connectionLocalAddr);
  if(m_useLMTP)
    m_helo = string("LHLO ") + *mailName + "\r\n";
  else
    m_helo = string("EHLO ") + *mailName + "\r\n";
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
  if(m_maxMsgSize > m_minMsgSize)
    size = random() % (m_maxMsgSize - m_minMsgSize) + m_minMsgSize;
  else
    size = m_maxMsgSize;
  m_md5.init();
  string logData;
  bool logAll = false;
  if(m_log && m_log->verbose())
    logAll = true;

  char aByte;
  rc = Read(&aByte, 1, 0);
  if(rc != 1)
    return 1;

  string sender = m_senderList->randomUser();
  string from = string("<") + sender + '>';
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
  char subj_buf[61];
  // random string from 2 to buffer len
  int subj_len = random() % (sizeof(subj_buf) - 3) + 2;
  m_data->randomString(subj_buf, subj_len);
  subj_buf[subj_len] = 0;
  subject += subj_buf;
  char date[34];
  m_data->date(date);
  string msgId = m_data->msgId(from.c_str(), getThreadNum());
  string str = string("From: ") + from + "\r\nTo: " + to + "\r\n"
                + subject + "Date: " + date + "Message-Id: " + msgId;
  m_md5.addData(str);

  char *sendbuf = (char *)malloc(size + 1);
  m_data->randomBuf(sendbuf, size);
  m_md5.addData(sendbuf, size);
  string sum = m_md5.getSum();
  str += "X-PostalHash: " + sum + m_data->postalMsg();
  rc = sendString(str);
  if(rc)
  {
    free(sendbuf);
    return rc;
  }
  if(logAll)
    logData = str;

  rc = sendData(sendbuf, size);
  if(rc)
    return rc;
  if(logAll)
    logData += sendbuf;
  free(sendbuf);
  str = ".\r\n";
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
    sum += " " + from + " " + to + "\n";
    m_log->Write(sum);
  }
  return 0;
}

ERROR_TYPE smtp::readCommandResp()
{
  char recvBuf[1024];
  do
  {
    int rc = readLine(recvBuf, sizeof(recvBuf));
    if(rc < 0)
      return ERROR_TYPE(rc);
    if(recvBuf[0] != '2' && recvBuf[0] != '3')
    {
      fprintf(stderr, "Server error:%s.\n", recvBuf);
      error();
      return eServer;
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
  return eNoError;
}

int smtp::pollRead()
{
  m_data->setRand(RAND_TIME);
  if(time(NULL) >= m_nextPrint)
  {
    m_res->print();
    m_nextPrint += 60;
  }
  return 0;
}

int smtp::WriteWork(PVOID buf, int size, int timeout)
{
  int t1 = time(NULL);
  if(t1 + timeout > m_nextPrint)
    timeout = m_nextPrint - t1;
  if(timeout < 0)
    timeout = 0;
  int rc = Write(buf, size, timeout);
  int t2 = time(NULL);
  if(t2 < t1 + timeout)
  {
    struct timespec req;
    req.tv_sec = t1 + timeout - t2;
    req.tv_nsec = 0;
    nanosleep(&req, NULL);
  }
  return rc;
}
