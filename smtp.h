#ifndef SMTP_H
#define SMTP_H

#include <string>
#include "tcp.h"

class results;

class UserList;

#define MAP_SIZE 8 * 1024

class smtpData
{
public:
  smtpData(const string &name, const char *app_name);
  ~smtpData();

  const string &helo() const { return m_helo; }
  const string &quit() const { return m_quit; }
  // return a random string that ends with "\r\n"
  string randomString(int max_len) const;
  string date() const;
  // return the X-Postal lines
  const string &postalMsg() const { return m_postalMsg; }

  // frequency determines how long it should have been since the last
  // operation for this function to actually do anything.
  // It will not sleep for that many seconds or block on IO in any way.
  void setRand(int frequency);

private:
  // Some const strings that we want initialised once before we get fork()ed.
  const string m_helo;
  const string m_quit;
  const char *m_randomLetters;
  const int m_randomLen;
  const string m_postalMsg;

  // time of the last randomise
  time_t m_timeLastAction;
  char m_randBuf[MAP_SIZE];
};

class smtp : public tcp
{
public:
  smtp(const char *addr, const char *ourAddr, const string &name
     , UserList &ul, int msgSize, int numMsgsPerConnection, int processes
     , Logit *log, TRISTATE netscape, bool ssl);

  virtual ~smtp();

  // connect returns 0 for connect, 1 for can't connect, and 2 for serious
  // errors.
  virtual int connect();
  int sendMsg();
  virtual int disconnect();
  int msgsPerConnection() const { return m_msgsPerConnection; }

private:
  virtual int action(PVOID param);

  smtp(int threadNum, const smtp *parent);
  virtual Fork *newThread(int threadNum);
  int pollRead();
  virtual int WriteWork(PVOID buf, int size, int timeout);

  virtual int readCommandResp();
  void error();
  virtual void sentData(int bytes);
  virtual void receivedData(int bytes);

  const string &m_name;
  UserList &m_ul;
  const int m_msgSize;
  smtpData *m_data;
  int m_msgsPerConnection;
  results *m_res;
  TRISTATE m_netscape;
  bool m_canTLS;
};

#endif
