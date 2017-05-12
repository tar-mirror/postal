#include "bhmusers.h"
#include <stdio.h>
#include "expand.h"

BHMUsers::BHMUsers(const char *userListFile)
{
  char buf[1024];
  FILE *fp = fopen(userListFile, "r");
  if(!fp)
  {
    printf("Can't open \"%s\".\n", userListFile);
    exit(1);
  }

  while(fgets(buf, sizeof(buf), fp) )
  {
    USER_SMTP_ACTION action = eNone;
    strtok(buf, "\n");
    BHM_DATA data;
    if(buf[0] && buf[0] != '#')
    {
      strtok(buf, " ");
      char *pass = strtok(NULL, " ");
      if(pass)
      {
        char *type_char = strtok(NULL, " ");
        if(type_char)
          action = chrToAction(type_char[0]);
      }
      data.action = action;
      data.sync_time = 0;
      m_map[buf] = data;
    }
  }
  if(m_map.size() == 0)
  {
    printf("No users in file.\n");
    exit(1);
  }
  fclose(fp);
}
