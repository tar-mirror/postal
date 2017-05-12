#include "userlist.h"
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include "expand.h"

UserList::UserList(const char *userListFile, bool usePass, bool stripDom)
 : m_users(new STR_VEC)
 , m_passwords(NULL)
 , m_index(0)
 , m_maxNameLen(0)
 , m_primary(true)
{
  char buf[1024];
  FILE *fp = fopen(userListFile, "r");
  if(!fp)
  {
    printf("Can't open \"%s\".\n", userListFile);
    exit(1);
  }

  if(usePass)
    m_passwords = new STR_VEC;

  while(fgets(buf, sizeof(buf), fp) )
  {
    strtok(buf, "\n");
    if(buf[0] != '\n' && buf[0] != '\r' && buf[0] != '#')
    {
      strtok(buf, " ");
      char *pass = strtok(NULL, " \t");
      if(!pass && usePass)
      {
        printf("Need a password for \"%s\".", buf);
        continue;
      }
      if(usePass)
        m_passwords->push_back(buf + strlen(buf) + 1);
      if(stripDom)
        strtok(buf, "@");
      m_users->push_back(buf);
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
  }
}

const string &UserList::randomUser()
{
  m_index = random() % m_users->size();
  return m_users[0][m_index];
}

string UserList::sequentialUser()
{
  m_index++;
  if(m_index == m_users->size())
    m_index = 0;
  return m_users[0][m_index];
}

string UserList::password()
{
  return m_passwords[0][m_index];
}
