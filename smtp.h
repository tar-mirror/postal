#ifndef SMTP_H
#define SMTP_H

using namespace std;
#include <string>
#include <time.h>
#include "conf.h"
#ifdef HAVE_EXT_HASH_MAP
using namespace __gnu_cxx;
#include <ext/hash_map>
#else
#include <hash_map.h>
#endif
#include "tcp.h"
#include "mutex.h"

class results;

class UserList;

#define MAP_SIZE 8 * 1024

// Comparison operator for hash map of names to unsigned longs
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
  smtpData();
  ~smtpData();

  const string &quit() const { return m_quit; }
  // return a random string that ends with "\r\n"
  string randomString(int max_len) const;
  const string date() const;
  const string msgId(const char *sender, const unsigned threadNum) const;

  // return the X-Postal lines
  const string &postalMsg() const { return m_postalMsg; }

  // frequency determines how long it should have been since the last
  // operation for this function to actually do anything.
  // It will not sleep for that many seconds or block on IO in any way.
  void setRand(int frequency);

  // get the mail name for an IP address
  const string * const getMailName(struct sockaddr_in &in);

private:
  // Some const strings that we only want one copy of
  const string m_quit;
  const char *m_randomLetters;
  const int m_randomLen;
  const string m_postalMsg;

  Mutex m_dnsLock;

  // time of the last randomise
  time_t m_timeLastAction;
  char m_randBuf[MAP_SIZE];

  // Map of IP addresses to names
  NAME_MAP m_names;

  smtpData(const smtpData&);
  smtpData & operator=(const smtpData&);
};

class smtp : public tcp
{
public:
  smtp(int *exitCount, const char *addr, const char *ourAddr
     , UserList &ul, UserList *senderList, int minMsgSize, int maxMsgSize
     , int numMsgsPerConnection, int processes, Logit *log, TRISTATE netscape
#ifdef USE_SSL
     , int ssl
#endif
     , unsigned short port, Logit *debug);

  virtual ~smtp();

  // Connect returns 0 for connect, 1 for can't connect, and 2 for serious
  // errors.
  int Connect();
  int sendMsg();
  virtual int disconnect();
  int msgsPerConnection() const { return m_msgsPerConnection; }

private:
  virtual int action(PVOID);

  smtp(int threadNum, const smtp *parent);
  virtual Thread *newThread(int threadNum);

  int pollRead();
  virtual int WriteWork(PVOID buf, int size, int timeout);

  virtual ERROR_TYPE readCommandResp(bool) { return readCommandResp(); }
  ERROR_TYPE readCommandResp();
  void error();
  virtual void sentData(int bytes);
  virtual void receivedData(int);

  UserList &m_ul, *m_senderList;
  const int m_minMsgSize, m_maxMsgSize;
  smtpData *m_data;
  int m_msgsPerConnection;
  results *m_res;
  TRISTATE m_netscape;
  string m_helo;
  time_t m_nextPrint;

  smtp(const smtp&);
  smtp & operator=(const smtp&);
};

#endif
