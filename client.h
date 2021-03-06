#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "tcp.h"

class clientResults;
class UserList;
class Mutex;

class client : public tcp
{
public:
  client(int *exitCount, const char *addr, const char *ourAddr, UserList &ul
       , int processes, int msgsPerConnection, Logit *log
#ifdef USE_SSL
    , int ssl
#endif
    , TRISTATE qmail_pop, int imap, int downloadPercent, int deletePercent
    , Logit *debug);
  client(int threadNum, const client *parent);

  virtual ~client();

  // Connect returns 0 for connect, 1 for can't connect, and 2 for serious
  // errors.
  int Connect(const string &user, const string &pass);
  virtual int disconnect();
  int list();
  int getMsg(int num, const string &user, bool log = false);
  void getUser(string &user, string &pass);

private:
  int connectPOP(const string &user, const string &pass);
  int connectIMAP(const string &user, const string &pass);
  virtual int action(PVOID);
  virtual Thread *newThread(int threadNum);

  virtual int pollRead();
  virtual int WriteWork(PVOID buf, int size, int timeout);

  virtual ERROR_TYPE readCommandResp(bool important = true);

  bool checkUser(const char *user);
  void error();
  virtual void sentData(int);
  virtual void receivedData(int bytes);
  virtual ERROR_TYPE sendCommandString(const string &s, bool important = true);

  UserList &m_ul;
  int m_maxNameLen;
  char *m_namesBuf;
  Mutex *m_sem;
  clientResults *m_res;
  int m_msgsPerConnection;
  int m_useIMAP;
  bool m_isIMAP;
  int m_imapID;
  char m_imapIDtxt[9];
  TRISTATE m_qmail_pop;
  int m_downloadPercent, m_deletePercent;

  client(const client&);
  client & operator=(const client&);
};

#endif
