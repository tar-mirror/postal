#ifndef ADDRESS_H
#define ADDRESS_H

#include <sys/types.h>
#include <netinet/in.h>
struct sockaddr;

class address
{
public:
  address(const char *addr, unsigned short default_port = 0);
  ~address();
  sockaddr *get_addr(int ind);
  sockaddr *get_rand_addr();
  int addressCount() const { return m_count; }

private:
  int resolve_name(int ind);
  sockaddr_in *m_addr;
  char **m_hostname;
  bool *m_rr_dns;
  int m_count;

  address(const address&);
  address & operator=(const address&);
};

#endif
