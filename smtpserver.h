#ifndef SMTP_SERVER_H
#define SMTP_SERVER_H

#include "logit.h"
#include "tcp.h"
#include "results.h"

using namespace std;

class UserList;

class smtp_server : public tcp
{
public:
  smtp_server(short port, UserList &ul, int maxMsgSize
            , int processes, Logit *log, Logit *debug
#ifdef USE_SSL
    , bool use_ssl
#endif
    );
  smtp_server(int threadNum, const smtp_server *parent);

  virtual ~smtp_server();

  virtual int disconnect();

  int doAllWork();

private:
  virtual int action(PVOID);
  virtual Thread *newThread(int threadNum);

  int pollRead();
  virtual int WriteWork(PVOID buf, int size, int timeout);

  virtual ERROR_TYPE readCommandResp(bool) { return readCommandResp(); }
  ERROR_TYPE readCommandResp();
  void error();
  virtual void sentData(int bytes);
  virtual void receivedData(int);

  static int m_processes;
  static int m_max_conn;
  UserList &m_ul;
  results *m_res;
  int m_maxMsgSize;
  static int m_nextThread;
};

#endif
