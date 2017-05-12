#define _GNU_SOURCE

#include "userlist.h"
#include "smtp.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/utsname.h>
#include <stdio.h>
#include "postal.h"
#include "logit.h"

void usage()
{
  printf("Usage: postal [-m maximum-message-size] [-p processes] [-l local-address]\n"
         "              [-c messages-per-connection] [-r messages-per-minute] [-a]\n"
         "              [-b [no]netscape] [-s]\n"
         "              smtp-server user-list-filename conversion-filename\n"
         "\n"
         "Postal Version: " VER_STR "\n");
  exit(1);
}

int main(int argc, char **argv)
{
  int maxMsgSize = 10;
  int processes = 1;
  int msgsPerMinute = 0; // 0 for unlimited
  int msgsPerConnection = 1;
  const char *ourAddr = NULL;
  bool allLog = false;
  TRISTATE netscape = eNONE;
  bool ssl = false;

  int c;
  while(-1 != (c = getopt(argc, argv, "ab:m:p:sc:r:l:")) )
  {
    switch(char(c))
    {
      case '?':
      case ':':
        usage();
      break;
      case 'a':
        allLog = true;
      break;
      case 'b':
        if(!strcasecmp(optarg, "netscape"))
          netscape = eWONT;
        else if(!strcasecmp(optarg, "nonetscape"))
          netscape = eMUST;
      break;
      case 'm':
        maxMsgSize = atoi(optarg);
      break;
      case 'p':
        processes = atoi(optarg);
      break;
      case 's':
        ssl = true;
      break;
      case 'c':
        msgsPerConnection = atoi(optarg);
      break;
      case 'r':
        msgsPerMinute = atoi(optarg);
      break;
      case 'l':
        ourAddr = optarg;
      break;
    }
  }
  if(maxMsgSize < 0 || maxMsgSize > MAX_MSG_SIZE || processes < 1
     || processes > MAX_PROCESSES || msgsPerMinute < 0 || msgsPerConnection < -1 )
    usage();

  if(optind + 3 > argc)
    usage();

  UserList ul(argv[optind + 1], argv[optind + 2]);

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

  Logit log("postal.log", allLog);
  smtp mailer(argv[optind], ourAddr, name, ul, maxMsgSize
            , msgsPerConnection, processes, &log, netscape, ssl);

  return mailer.doAllWork(msgsPerMinute);
}

