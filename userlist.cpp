#include "userlist.h"
#include <stdio.h>
#include "expand.h"

UserList::UserList(const char *userListFile, const char *conversionFile
                 , bool usePassword)
 : m_users(new STR_VEC)
 , m_passwords(new STR_VEC)
 , m_exp(new NameExpand(conversionFile))
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
 , m_exp(list.m_exp)
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
    delete m_exp;
  }
}

string UserList::randomUser()
{
  string str;
  m_index = random() % m_users->size();
  m_exp->expand(str, m_users[0][m_index]);
  return str;
}

string UserList::sequentialUser()
{
  string str;
  while(m_exp->expand(str, m_users[0][m_index], true))
  {
    m_index++;
    if(m_index == m_users->size())
      m_index = 0;
  }
  return str;
}

string UserList::password()
{
  return m_passwords[0][m_index];
}
