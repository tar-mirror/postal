#ifndef SMTP_H
#define SMTP_H

#include <string>
#include <time.h>
#include <hash_map.h>
#include "tcp.h"
#include "mutex.h"

class results;

class UserList;

#define MAP_SIZE 8 * 1024

struct eqlng
{
  bool operator()(unsigned long l1, unsigned long l2) const
  {
    return (l1 == l2);
  }
};

typedef hash_map<unsigned long, string *, hash<unsigned long>, eqlng> NAME_MAP;

class smtpData
{
public:
  smtpData(const char *app_name);
  ~smtpData();

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

  // get the mail name for an IP address
  const string * const getMailName(struct sockaddr_in &in);

private:
  // Some const strings that we want initialised once before we get fork()ed.
  const string m_quit;
  const char *m_randomLetters;
  const int m_randomLen;
  const string m_postalMsg;
  string m_mailname;

  Mutex m_dnsLock;

  // time of the last randomise
  time_t m_timeLastAction;
  char m_randBuf[MAP_SIZE];
  NAME_MAP m_names;
};

class smtp : public tcp
{
public:
  smtp(const char *addr, const char *ourAddr
     , UserList &ul, int msgSize, int numMsgsPerConnection, int processes
     , Logit *log, TRISTATE netscape
#ifdef USE_SSL
     , bool ssl
#endif
      );

  virtual ~smtp();

  // connect returns 0 for connect, 1 for can't connect, and 2 for serious
  // errors.
  int connect();
  int sendMsg();
  virtual int disconnect();
  int msgsPerConnection() const { return m_msgsPerConnection; }

private:
  virtual int action(PVOID param);

  smtp(int threadNum, const smtp *parent);
  virtual Fork *newThread(int threadNum);
  int pollRead();
  virtual int WriteWork(PVOID buf, int size, int timeout);

  virtual int readCommandResp(bool important = true);
  void error();
  virtual void sentData(int bytes);
  virtual void receivedData(int bytes);

  UserList &m_ul;
  const int m_msgSize;
  smtpData *m_data;
  int m_msgsPerConnection;
  results *m_res;
  TRISTATE m_netscape;
  string m_helo;
#ifdef USE_SSL
  bool m_canTLS;
#endif
};

#endif
