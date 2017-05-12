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
         "              [-b [no]netscape] [-[z|Z] debug-file]\n"
#ifdef USE_SSL
         "              [-s ssl-percentage]\n"
#endif
         "              smtp-server user-list-filename conversion-filename\n"
         "\n"
         "Postal Version: " VER_STR "\n");
  exit(eParam);
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
  PCCHAR debugName = NULL;
  bool debugMultipleFiles = false;
#ifdef USE_SSL
  int ssl = 0;
#endif

  int c;
  optind = 0;
  while(-1 != (c = getopt(argc, argv, "ab:m:p:s:c:r:l:z:Z:")) )
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
      case 'Z':
        debugMultipleFiles = true;
      case 'z':
        debugName = optarg;
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
    return eSystem;
  }
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_sigaction = NULL;
  sa.sa_flags = 0;
  if(sigaction(SIGPIPE, &sa, NULL))
  {
    printf("Can't block SIGPIPE.\n");
    return eSystem;
  }
  printf("time,messages,data(K),errors,connections"
#ifdef USE_SSL
    ",SSL connections"
#endif
    "\n");

  Logit log("postal.log", allLog, false, 0);
  Logit *debug = NULL;

  if(debugName)
    debug = new Logit(debugName, false, debugMultipleFiles, 0);

  smtp mailer(argv[optind], ourAddr, ul, maxMsgSize
            , msgsPerConnection, processes, &log, netscape
#ifdef USE_SSL
            , ssl
#endif
            , debug);

  return mailer.doAllWork(msgsPerMinute);
}

