#ifndef USERLIST_H
#define USERLIST_H

using namespace std;

#include "conf.h"

#ifdef HAVE_VECTOR
#include <vector>
#else
#include <vector.h>
#endif

#include <string>

typedef vector<string> STR_VEC;

#include "postal.h"

class UserList
{
public:
  UserList(const char *userListFile, bool usePass, bool stripDom = false);
  UserList(UserList &list);
  ~UserList();

  string randomUser();
  string password(); // get the password from the last user we got
  string sequentialUser();
  size_t maxNameLen() const { return m_maxNameLen; }

private:
  STR_VEC *m_users;
  STR_VEC *m_passwords;
  unsigned int m_index;
  size_t m_maxNameLen;
  bool m_primary;

  UserList(const UserList&);
  UserList & operator=(const UserList&);
};

#endif
