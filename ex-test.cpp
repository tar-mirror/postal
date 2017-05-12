#include "expand.h"
#include <string>
#include <stdio.h>

void usage()
{
  printf("Usage: ex-test input-file conversion-file\n");
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
      for(int i = 0; i < 1000; i++)
      {
        string str;
        if(exp.expand(str, buf))
          return 1;
        printf("%s\n", str.c_str());
      }
    }
  }
  fclose(fp);
  return 0;
}
