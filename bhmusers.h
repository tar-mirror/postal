#ifndef USERLIST_H
#define USERLIST_H

using namespace std;

#include <string>
#include "conf.h"

#ifdef HAVE_EXT_HASH_MAP
using namespace __gnu_cxx;
#include <ext/hash_map>
#else
#include <hash_map.h>
#endif

#include "postal.h"

typedef enum { eNone = 0, eDefer, eReject, eBounce, eGrey } USER_SMTP_ACTION;

typedef struct
{
  USER_SMTP_ACTION action;
  int sync_time;
} BHM_DATA;

namespace __gnu_cxx
{
  template<> struct hash< std::string >
  {
    size_t operator() ( const std::string &x ) const
    {
      return hash < const char * >() ( x.c_str() );
    }
  };
}

typedef hash_map<string, BHM_DATA , hash<string> > NAME_MAP;

class BHMUsers
{
public:
  BHMUsers(const char *userListFile);
  ~BHMUsers() {};

private:
  BHMUsers(BHMUsers &list);

  NAME_MAP m_map;

  USER_SMTP_ACTION chrToAction(char c)
  {
    switch(c)
    {
    case 'd':
      return eDefer;
    case 'r':
      return eReject;
    case 'b':
      return eBounce;
    case 'g':
      return eGrey;
    }
    return eNone;
  }

  BHMUsers(const BHMUsers&);
  BHMUsers & operator=(const BHMUsers&);
};

#endif
