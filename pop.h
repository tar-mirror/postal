#ifndef SMTP_H
#define SMTP_H

#include <string>
#include "tcp.h"

class results;
class UserList;
class Mutex;

class pop : public tcp
{
public:
  pop(const char *addr, const char *ourAddr, UserList &ul, int processes
    , int msgsPerConnection, Logit *log, bool ssl);
  pop(int threadNum, const pop *parent);

  virtual ~pop();

  // connect returns 0 for connect, 1 for can't connect, and 2 for serious
  // errors.
  virtual int connect(const string &user, const string &pass);
  virtual int disconnect();
  int list();
  int getMsg(int num, const string &user, bool log = false);
  void getUser(string &user, string &pass);

private:
  virtual int action(PVOID param);
  virtual Fork *newThread(int threadNum);

  virtual int pollRead();
  virtual int WriteWork(PVOID buf, int size, int timeout);

  virtual int readCommandResp();

  bool checkUser(const char *user);
  void error();
  virtual void sentData(int bytes);
  virtual void receivedData(int bytes);

  UserList &m_ul;
  int m_maxNameLen;
  char *m_namesBuf;
  Mutex *m_sem;
  results *m_res;
  int m_threadNum;
  int m_msgsPerConnection;
};

#endif
