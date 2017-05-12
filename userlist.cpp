#include "userlist.h"
#include <stdio.h>
#ifndef NO_CONVERSION
#include "expand.h"
#endif

UserList::UserList(const char *userListFile
#ifndef NO_CONVERSION
                 , const char *conversionFile
#endif
                 , bool usePassword)
 : m_users(new STR_VEC)
 , m_passwords(new STR_VEC)
#ifndef NO_CONVERSION
 , m_exp(new NameExpand(conversionFile))
#endif
 , m_index(0)
 , m_maxNameLen(0)
 , m_primary(true)
{
  FILE *fp = fopen(userListFile, "r");
  if(!fp)
  {
    printf("Can't open \"%s\".\n", userListFile);
    exit(1);
  }
  char buf[1024];
  while(fgets(buf, sizeof(buf), fp) )
  {
    strtok(buf, "\n");
    if(buf[0] && buf[0] != '#' && buf[0] != '\n')
    {
      if(usePassword)
      {
        size_t len = strlen(buf);
        strtok(buf, " ");
        if(len > strlen(buf))
        {
          m_users->push_back(buf);
          m_passwords->push_back(buf + strlen(buf) + 1);
        }
      }
      else
      {
        m_users->push_back(buf);
      }
      if(strlen(buf) > m_maxNameLen)
        m_maxNameLen = strlen(buf);
    }
  }
  if(m_users->size() == 0)
  {
    printf("No users in file.\n");
    exit(1);
  }
  fclose(fp);
}

UserList::UserList(UserList &list)
 : m_users(list.m_users)
 , m_passwords(list.m_passwords)
#ifndef NO_CONVERSION
 , m_exp(list.m_exp)
#endif
 , m_index(0)
 , m_maxNameLen(0)
 , m_primary(false)
{
}

UserList::~UserList()
{
  if(m_primary)
  {
    delete m_users;
    delete m_passwords;
#ifndef NO_CONVERSION
    delete m_exp;
#endif
  }
}

string UserList::randomUser()
{
  m_index = random() % m_users->size();
#ifndef NO_CONVERSION
  string str;
  m_exp->expand(str, m_users[0][m_index]);
  return str;
#else
  return m_users[0][m_index];
#endif
}

string UserList::sequentialUser()
{
#ifndef NO_CONVERSION
  string str;
  while(m_exp->expand(str, m_users[0][m_index], true))
  {
    m_index++;
    if(m_index == m_users->size())
      m_index = 0;
  }
  return str;
#else
  m_index++;
  if(m_index == m_users->size())
    m_index = 0;
  return m_users[0][m_index];
#endif
}

string UserList::password()
{
  return m_passwords[0][m_index];
}
