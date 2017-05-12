#include "smtpserver.h"

int smtp_server::m_processes = 0;
int smtp_server::m_max_conn = 0;
int smtp_server::m_nextThread = 0;


smtp_server::smtp_server(short port, UserList &ul, int maxMsgSize
                       , int processes, Logit *log, Logit *debug
#ifdef USE_SSL
    , bool use_ssl
#endif
    )
 : tcp(NULL, port, log
#ifdef USE_SSL
     , use_ssl
#endif
     , debug)
 , m_ul(ul)
 , m_res(new results)
 , m_maxMsgSize(maxMsgSize)
{
  m_max_conn = processes;
}

smtp_server::smtp_server(int threadNum, const smtp_server *parent)
 : tcp(threadNum, parent)
 , m_ul(parent->m_ul)
 , m_res(parent->m_res)
 , m_maxMsgSize(parent->m_maxMsgSize)
{
}


Thread *smtp_server::newThread(int threadNum)
{
  return new smtp_server(threadNum, this);
}

smtp_server::~smtp_server()
{
}

int smtp_server::action(PVOID)
{
  while(1)
  {
    
  }
}

int smtp_server::pollRead()
{
  m_res->pollPrint();
  if(m_max_conn && m_processes < m_max_conn)
  {
    int rc = pollForAccept();
    if(rc == -1)
      return -1;
    if(rc == 1)
    {
      smtp_server *n = (smtp_server *)newThread(m_nextThread);
      if(n->isOpen())
      {
        m_nextThread++;
        m_processes++;
      }
      else
        delete n;
    }
  }
  return 0;
}

void smtp_server::error()
{
  m_res->error();
  tcp::disconnect();
}

int smtp_server::WriteWork(PVOID buf, int size, int timeout)
{
  return Write(buf, size, timeout);
}

void smtp_server::sentData(int bytes)
{
  m_res->dataBytes(bytes);
}

ERROR_TYPE smtp_server::readCommandResp()
{
  char recvBuf[1024];
  
}

int smtp_server::doAllWork()
{
  
}

int smtp_server::disconnect()
{
  const char * str = "550 disconnect\r\n";
  const len = strlen(str);
  rc = sendData(str, len);
  if(rc != len)
    return -1;
  rc = 
}
