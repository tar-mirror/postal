#include "expand.h"
#include <cstdlib>
#include <string>
#include <stdio.h>


void usage()
{
  printf("Usage: postal-list input-file conversion-file\n");
  exit(1);
}

int main(int argc, char **argv)
{
  if(argc != 3)
    usage();
  NameExpand exp(argv[2]);
  FILE *fp = fopen(argv[1], "r");
  if(!fp)
    usage();
  char buf[1024];
  while(fgets(buf, sizeof(buf), fp) )
  {
    if(buf[0] && buf[0] != '\n')
    {
      strtok(buf, "\n");
      string str;
      while(!exp.expand(str, buf, true))
      {
        printf("%s\n", str.c_str());
      }
    }
  }
  fclose(fp);
  return 0;
}
