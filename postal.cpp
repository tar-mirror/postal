#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "userlist.h"
#include "smtp.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include "postal.h"
#include "logit.h"
#ifdef USE_GNUTLS
#include <errno.h>
#include <gcrypt.h>
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

void usage()
{
  printf("Usage: postal [-m maximum-message-size] [-M minimum-message-size] [-t threads]\n"
         "              [-c messages-per-connection] [-r messages-per-minute] [-a]\n"
         "              [-b [no]netscape] [-p port] [-[z|Z] debug-file]\n"
#ifdef USE_SSL
         "              [-s ssl-percentage]\n"
#endif
         "              [-l local-address] [-f sender-file]\n"
         "              smtp-server user-list-filename\n"
         "\n"
         "Postal Version: " VER_STR "\n");
  exit(eParam);
}

int exitCount = 0;

void endit(int)
{
  exitCount++;
  if(exitCount > 2)
    exit(1);
}

int main(int argc, char **argv)
{
  int maxMsgSize = 10;
  int minMsgSize = 0;
  int processes = 1;
  int msgsPerMinute = 0; // 0 for unlimited
  int msgsPerConnection = 1;
  const char *ourAddr = NULL;
  bool allLog = false;
  TRISTATE netscape = eNONE;
  PCCHAR debugName = NULL;
  PCCHAR senderFile = NULL;
  bool debugMultipleFiles = false;
#ifdef USE_SSL
  int ssl = 0;
#endif
#ifdef USE_GNUTLS
  gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
  gnutls_global_init();
  if(!gnutls_check_version(GNUTLS_VER))
  {
    fprintf(stderr, "Needs version " GNUTLS_VER " of GNUTLS\n");
    exit(1);
  }
#endif
  unsigned short port = 25;

  int c;
  while(-1 != (c = getopt(argc, argv, "ab:f:m:M:p:s:t:c:r:l:z:Z:")) )
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
      case 'f':
          senderFile = optarg;
      break;
      case 'm':
        maxMsgSize = atoi(optarg);
      break;
      case 'M':
        minMsgSize = atoi(optarg);
      break;
      case 'p':
        port = atoi(optarg);
      break;
      case 's':
#ifdef USE_SSL
        ssl = atoi(optarg);
#else
        usage();
#endif
      break;
      case 't':
        processes = atoi(optarg);
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
  if(minMsgSize < 0 || maxMsgSize > MAX_MSG_SIZE || maxMsgSize < minMsgSize)
    usage();

  if(processes < 1 || processes > MAX_PROCESSES)
    usage();
  if(msgsPerMinute < 0 || msgsPerConnection < -1 )
    usage();
#ifdef USE_SSL
  if(ssl < 0 || ssl > 100)
    usage();
#endif

  if(optind + 2 != argc)
    usage();

  UserList ul(argv[optind + 1], false);
  UserList *senderList = NULL;
  if(senderFile)
    senderList = new UserList(senderFile, false);

  int fd[2];
  if(pipe(fd))
  {
    printf("Can't create pipe.\n");
    return eSystem;
  }
  struct sigaction sa;
  sa.sa_sigaction = NULL;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  if(sigaction(SIGPIPE, &sa, NULL))
  {
    printf("Can't block SIGPIPE.\n");
    return eSystem;
  }

  sa.sa_flags = SA_SIGINFO;
  sa.sa_handler = &endit;
  if(sigaction(SIGINT, &sa, NULL))
  {
    printf("Can't handle SIGINT.\n");
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

  smtp mailer(&exitCount, argv[optind], ourAddr, ul, senderList, minMsgSize
            , maxMsgSize, msgsPerConnection, processes, &log, netscape
#ifdef USE_SSL
            , ssl
#endif
            , port, debug);

  return mailer.doAllWork(msgsPerMinute);
}

