#define _GNU_SOURCE

#include "userlist.h"
#include "smtp.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include "postal.h"
#include "logit.h"

void usage()
{
  printf("Usage: postal [-m maximum-message-size] [-p processes] [-l local-address]\n"
         "              [-c messages-per-connection] [-r messages-per-minute] [-a]\n"
         "              [-b [no]netscape]\n"
#ifdef USE_SSL
         "              [-s ssl-percentage]\n"
#endif
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
#ifdef USE_SSL
  int ssl = 0;
#endif

  int c;
  while(-1 != (c = getopt(argc, argv, "ab:m:p:s:c:r:l:")) )
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
#ifdef USE_SSL
        ssl = atoi(optarg);
#else
        usage();
#endif
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
  if(maxMsgSize < 0 || maxMsgSize > MAX_MSG_SIZE)
    usage();

  if(processes < 1 || processes > MAX_PROCESSES)
    usage();
  if(msgsPerMinute < 0 || msgsPerConnection < -1 )
    usage();
#ifdef USE_SSL
  if(ssl < 0 || ssl > 100)
    usage();
#endif

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
  sa.sa_handler = SIG_IGN;
  sa.sa_sigaction = NULL;
  sa.sa_flags = 0;
  if(sigaction(SIGPIPE, &sa, NULL))
  {
    printf("Can't block SIGPIPE.\n");
    return 1;
  }
  printf("time,messages,data(K),errors,connections"
#ifdef USE_SSL
    ",SSL connections"
#endif
    "\n");

  Logit log("postal.log", allLog);
  smtp mailer(argv[optind], ourAddr, ul, maxMsgSize
            , msgsPerConnection, processes, &log, netscape
#ifdef USE_SSL
            , ssl);
#else
            );
#endif

  return mailer.doAllWork(msgsPerMinute);
}

