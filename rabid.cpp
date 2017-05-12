#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "userlist.h"
#include "client.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include "postal.h"
#include "logit.h"

void usage()
{
  printf("Usage: rabid [-r max-connections-per-minute] [-p processes] [-l local-address]\n"
         "             [-c messages-per-connection] [-a] [-i imap-percentage]\n"
#ifdef USE_SSL
         "             [-s ssl-percentage] [-d download-percentage[:delete-percentage]]\n"
#endif
         "             [-[z|Z] debug-file]\n"
         "             pop-server user-list-filename conversion-filename\n"
         "\n"
         "Rabid Version: " VER_STR "\n");
  exit(eParam);
}

int main(int argc, char **argv)
{

  int processes = 1;
  int connectionsPerMinute = 0;
  int msgsPerConnection = -1;
  const char *ourAddr = NULL;
  bool logAll = false;
#ifdef USE_SSL
  int ssl = 0;
#endif
  int imap = 0;
  int downloadPercent = 100, deletePercent = 100;
  TRISTATE qmail_pop = eNONE;
  PCCHAR debugName = NULL;
  bool debugMultipleFiles = false;

  int c;
  while(-1 != (c = getopt(argc, argv, "ab:d:c:i:l:p:r:s:z:Z:")) )
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
      case 'b':
        if(!strcasecmp(optarg, "qmail-pop"))
          qmail_pop = eWONT;
      break;
      case 'c':
        msgsPerConnection = atoi(optarg);
      break;
      case 'd':
        if(sscanf(optarg, "%d:%d", &downloadPercent, &deletePercent) < 2)
          deletePercent = 100;
      break;
      case 'i':
        imap = atoi(optarg);
      break;
      case 'l':
        ourAddr = optarg;
      break;
      case 'p':
        processes = atoi(optarg);
      break;
      case 'r':
        connectionsPerMinute = atoi(optarg);
      break;
      case 's':
#ifdef USE_SSL
        ssl = atoi(optarg);
#else
        usage();
#endif
      break;
      case 'Z':
        debugMultipleFiles = true;
      case 'z':
        debugName = optarg;
      break;
    }
  }
  if(processes < 1 || processes > MAX_PROCESSES || connectionsPerMinute < 0)
    usage();
#ifdef USE_SSL
  if(ssl < 0 || ssl > 100)
    usage();
#endif
  if(imap < 0 || imap > 100)
    usage();
  if(downloadPercent < 0 || downloadPercent > 100)
    usage();
  if(deletePercent < 0 || deletePercent > 100)
    usage();
  if(optind + 3 != argc)
    usage();

  UserList ul(argv[optind + 1]
            , argv[optind + 2]
            , true);

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
         ",SSL connections,"
#endif
         "IMAP connections\n");
  Logit log("rabid.log", logAll, false, 0);
  Logit *debug = NULL;
 
  if(debugName)
    debug = new Logit(debugName, false, debugMultipleFiles, 0);
  client popper(argv[optind], ourAddr, ul, processes, msgsPerConnection, &log
#ifdef USE_SSL
              , ssl
#endif
              , qmail_pop, imap, downloadPercent, deletePercent, debug);

  return popper.doAllWork(connectionsPerMinute);
}

