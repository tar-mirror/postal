#define _GNU_SOURCE

#include "userlist.h"
#include "pop.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/utsname.h>
#include <stdio.h>
#include "postal.h"
#include "logit.h"

void usage()
{
  printf("Usage: rabid [-r max-connections-per-minute] [-p processes] [-l local-address]\n"
         "             [-c messages-per-connection] [-a] [-s]\n"
         "             pop-server user-list-filename conversion-filename\n"
         "\n"
         "Rabid Version: " VER_STR "\n");
  exit(1);
}

int main(int argc, char **argv)
{

  int processes = 1;
  int connectionsPerMinute = 0;
  int msgsPerConnection = -1;
  const char *ourAddr = NULL;
  bool logAll = false;
  bool ssl = false;

  int c;
  while(-1 != (c = getopt(argc, argv, "ap:sr:l:c:")) )
  {
    switch(char(c))
    {
      case '?':
      case ':':
        usage();
      break;
      case 'a':
        logAll = true;
      break;
      case 'p':
        processes = atoi(optarg);
      break;
      case 'r':
        connectionsPerMinute = atoi(optarg);
      break;
      case 's':
        ssl = true;
      break;
      case 'l':
        ourAddr = optarg;
      break;
      case 'c':
        msgsPerConnection = atoi(optarg);
      break;
    }
  }
  if(processes < 1 || processes > MAX_PROCESSES || connectionsPerMinute < 0)
    usage();
  if(optind + 3 > argc)
    usage();

  UserList ul(argv[optind + 1], argv[optind + 2], true);

  int fd[2];
  if(pipe(fd))
  {
    printf("Can't create pipe.\n");
    return 1;
  }
  struct sigaction sa;
  sa.sa_handler = NULL;
  sa.sa_sigaction = NULL;
  __sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_restorer = NULL;
  if(sigaction(SIGPIPE, &sa, NULL))
  {
    printf("Can't block SIGPIPE.\n");
    return 1;
  }
  utsname uts;
  if(uname(&uts))
  {
    printf("Unable to get name of this host.\n");
    return 1;
  }
  string name = uts.nodename;
  if(strlen(uts.domainname))
  {
    name += '.';
    name += uts.domainname;
  }
  printf("time,messages,data(K),errors,connections\n");
  Logit log("rabid.log", logAll);
  pop popper(argv[optind], ourAddr, ul, processes, msgsPerConnection, &log, ssl);

  return popper.doAllWork(connectionsPerMinute);
}

