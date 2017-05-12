#include "address.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>

typedef char *PCHAR;

address::address(const char *addr, unsigned short default_port)
{
  unsigned short port = default_port;
  char *addr_copy = strdup(addr);
  int len = strlen(addr);
  m_count = 1;
  int i;
  for(i = 0; i < len; i++)
  {
    if(addr_copy[i] == ',')
      m_count++;
  }
  char **names = new PCHAR[m_count];
  int ind = 0;
  for(i = 0; i < m_count; i++)
  {
    names[i] = &addr_copy[ind];
    while(addr_copy[ind] && addr_copy[ind] != ',')
      ind++;
    addr_copy[ind] = '\0';
    ind++;
  }
  m_rr_dns = new bool[m_count];
  m_addr = new sockaddr_in[m_count];
  m_hostname = new PCHAR[m_count];
  for(i = 0; i < m_count; i++)
  {
    char *hostname = names[i];
    if(hostname[0] == '+')
    {
      m_rr_dns[i] = true;
      hostname++;
    }
    else
    {
      m_rr_dns[i] = false;
    }
    if(hostname[0] == '[')
    {
      hostname++;
      int j;
      for(j = 0; hostname[j] && hostname[j] != ']'; j++);
 
      if(!hostname[j])
      {
        fprintf(stderr, "Bad address: %s\n", addr);
        exit(1);
      }
      hostname[j] = '\0';
      port = atoi(&hostname[j + 1]);
    }
    m_hostname[i] = strdup(hostname);
    m_addr[i].sin_family = AF_INET;
    m_addr[i].sin_port = htons(port);
    if(!m_rr_dns[i])
    {
      if(resolve_name(i))
        exit(1);
    }
  }
  free(addr_copy);
}

address::~address()
{
  for(int i = 0; i < m_count; i++)
    free(m_hostname[i]);
  delete m_hostname;
  delete m_rr_dns;
  delete m_addr;
}

int address::resolve_name(int ind)
{
  hostent *he = gethostbyname(m_hostname[ind]);
  if(!he)
  {
    fprintf(stderr, "Bad address \"%s\" for server.\n", m_hostname[ind]);
    return 1;
  }
  m_addr[ind].sin_addr.s_addr = *((u_int32_t *)he->h_addr_list[0]);
  return 0;
}

sockaddr *address::get_rand_addr()
{
  int ind = 0;
  if(m_count > 1)
    ind = rand() % m_count;
  return get_addr(ind);
}

sockaddr *address::get_addr(int ind)
{
  if(ind < 0 || ind > m_count)
    return get_rand_addr();
  if(m_rr_dns[ind])
  {
    if(resolve_name(ind))
      return NULL;
  }
  return (sockaddr *)&m_addr[ind];
}
