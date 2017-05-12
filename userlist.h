#ifndef USERLIST_H
#define USERLIST_H

#include <vector.h>
#include <string>

typedef vector<string> STR_VEC;

#include "postal.h"

#ifndef NO_CONVERSION
class NameExpand;
#endif

class UserList
{
public:
  UserList(const char *userListFile
#ifndef NO_CONVERSION
         , const char *conversionFile
#endif
         , bool usePassword = false);
  UserList(UserList &list);
  ~UserList();

  string randomUser();
  string password(); // get the password from the last user we got
  string sequentialUser();
  size_t maxNameLen() const { return m_maxNameLen; }

private:
  STR_VEC *m_users;
  STR_VEC *m_passwords;
#ifndef NO_CONVERSION
  NameExpand *m_exp;
#endif
  unsigned int m_index;
  size_t m_maxNameLen;
  bool m_primary;
};

#endif
