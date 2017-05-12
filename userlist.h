#ifndef USERLIST_H
#define USERLIST_H

#include <vector.h>
#include <string>

typedef vector<string> STR_VEC;

#include "postal.h"

class NameExpand;

class UserList
{
public:
  UserList(const char *userListFile
         , const char *conversionFile
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
  NameExpand *m_exp;
  unsigned int m_index;
  size_t m_maxNameLen;
  bool m_primary;

  UserList(const UserList&);
  UserList & operator=(const UserList&);
};

#endif
