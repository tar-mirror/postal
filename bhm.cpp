#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "bhmusers.h"
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "postal.h"
#include "logit.h"
#include "results.h"
#include "basictcp.h"
#ifdef USE_GNUTLS
#include <errno.h>
#include <gcrypt.h>
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

int processes = 0;
int *thread_status;

void usage(CPCCHAR msg = NULL)
{
  if(msg)
    printf("Error: %s\n\n", msg);

  printf("Usage: bhm [-m maximum-message-size] [-t threads] [-p port]"
#ifdef USE_SSL
         " [-s]"
#endif
         "\n"
         "              [-a] [-(z|Z) debug-file] user-list-filename\n"
         "\n"
         "Postal Version: " VER_STR "\n");
  close(1);
  exit(eParam);
}

int maxMsgSize = 10240;
results res;
Logit *log;

int exitCount = 0;

void endit(int)
{
  exitCount++;
  if(exitCount > 2)
    exit(1);
}

typedef struct
{
  int threadNum;
  int fd;
  struct sockaddr_in addr;
  int ssl;
  Logit *debug;
} thread_data;

enum { eFree = 0, eUsed, eFinished };

int check_sender(CPCCHAR addr)
{
  return 0;
}

int readCommand(base_tcp &t, char *buf, int bufSize, bool stripCR, int timeout = 60);

// return 1 for successfully accepting an address, 0 for quit, <0 for error
// return 2 for data
int recv_addr(base_tcp &t, char *buf, const size_t buf_len, CPCCHAR msg, string &addr, bool do_data)
{
  while(1)
  {
    char *tmp;
    bool syntax_warn = false;
    int len = readCommand(t, buf, buf_len, true);
    if(len < 0)
      return len;
    if(!strncasecmp(buf, "quit", 4) && (isblank(buf[4]) || buf[4] == '\0'))
      return 0;
    if(do_data && !strncasecmp(buf, "data", 4) && (len == 4 || (isblank(buf[4])  && len == 5)))
      return 2;
    if(!strncasecmp(buf, msg, strlen(msg)))
    {
      CPCCHAR bad_sender_msg = "501 Bad address syntax\r\n";
      if(buf[len - 1] == '\r')
      {
        len--;
        buf[len] = '\0';
      }
      tmp = buf + strlen(msg);
      while(isblank(*tmp))
        tmp++;
      if(*tmp != '<' && buf[len - 1] != '>')
      {
        syntax_warn = true;
      }
      else if(*tmp != '<' || buf[len - 1] != '>')
      {
        t.sendCstr(bad_sender_msg);
        res.error();
        continue;
      }
      else
      {
        tmp++;
        len--;
        buf[len] = '\0';
      }
      if(check_sender(tmp))
      {
        t.sendCstr(bad_sender_msg);
        res.error();
        continue;
      }
      addr = tmp;
      if(syntax_warn)
        t.sendCstr("250 Ok but you should use <addr>\r\n");
      else
        t.sendCstr("250 Ok\r\n");
      break;
    }
    res.error();
    int rc = t.sendCstr("502 Error: Command not recognised or not appropriate at this time\r\n");
    if(rc < 0)
      return rc;
  }
  return 1;
}

// return 1 for successfully accepting a message, 0 for quit, -1 for error
int recv_msg(base_tcp &t, char *buf, const size_t buf_len)
{
  string sender;
  string recipient;
  int rc = recv_addr(t, buf, buf_len, "mail from:", sender, false);
  if(rc != 1)
    return rc;

  int recipient_count = 0;
  while( (rc = recv_addr(t, buf, buf_len, "rcpt to:", recipient, recipient_count)) == 1)
  {
    recipient_count++;
  }
  if(rc <= 0)
    return rc;
  t.sendCstr("354 End data with <CR><LF>.<CR><LF>\r\n");
  while( (rc = readCommand(t, buf, buf_len, true)) >= 0)
  {
    if(!strcmp(".", buf))
      break;
  }
  if(rc < 0)
  {
    res.error();
    return rc;
  }
  struct timespec req;
  req.tv_sec = 0;
  req.tv_nsec = 1000000;
  nanosleep(&req, NULL);
  t.sendCstr("250 Ok: queued on /dev/null\r\n");
  res.message();
  return 1;
}

const char *hostname = "bhm.coker.com.au";

int do_helo(base_tcp &t, char *buf, const size_t buf_len)
{
  int len;
  while(1)
  {
    int rc;
    if( (len = t.readLine(buf, buf_len, true)) < 0)
    {
      res.error();
      return -1;
    }
    if(buf[len - 1] == '\r')
    {
      len--;
      buf[len] = '\0';
    }
    bool basicHello = false;
    if(!strncasecmp("quit", buf, 4))
    {
      t.sendCstr("221 Bye\r\n");
      return -2;
    }
    if(!strncasecmp("helo", buf, 4))
    {
      basicHello = true;
    }
    else if(strncasecmp("ehlo", buf, 4))
    {
      res.error();
      if(t.sendCstr("502 Error: command not recognized\r\n") < 0)
        return -3;
      continue;
    }
    if(len < 6 || !isblank(buf[4]))
    {
      res.error();
      if(t.sendCstr("501 Syntax: EHLO hostname or HELO hostname\r\n") < 0)
        return -3;
      continue;
    }
    string remoteName(buf + 5);
    if(basicHello)
      rc = t.printf("250 %s\r\n", hostname);
    else
      rc = t.printf("250-%s\r\n250-PIPELINING\r\n"
#ifdef USE_SSL
                    "250-STARTTLS\r\n"
#endif
                    "250 SIZE %d\r\n", hostname, maxMsgSize * 1024);
    if(rc < 0)
    {
      res.error();
      return -1;
    }
    break;
  }
  return 0;
}

int readCommand(base_tcp &t, char *buf, int bufSize, bool stripCR, int timeout)
{
#ifdef USE_SSL
  if(!t.isTLS())
  {
    int rc = t.readLine(buf, bufSize, stripCR, timeout);
    if(rc < 0 || strcasecmp(buf, "starttls"))
      return rc;
    t.printf("220 2.0.0 Ready to start TLS\r\n");
    if(t.ConnectTLS())
      return -1;
    res.connect_ssl();
    rc = do_helo(t, buf, bufSize);
    if(rc < 0)
      return rc; // need to make sure values are consistent
    rc = t.readLine(buf, bufSize, stripCR, timeout);
    return rc;
  }
  else
#endif
  return t.readLine(buf, bufSize, stripCR, timeout);
}

void do_work(thread_data *td)
{
  base_tcp t(td->fd, log, td->debug, &res
#ifdef USE_SSL
    , td->ssl
#endif
  );
  struct sockaddr_in name;
  socklen_t namelen = sizeof(name);
  char buf[1024];

  int rc = getsockname(td->fd, (sockaddr *)&name, &namelen);

  if(rc)
  {
    fprintf(stderr, "Error getting socket name.\n");
    return;
  }
/*  if(!inet_ntop(name.sin_family, &name.sin_addr, buf, sizeof(buf)))
    printf("error decoding\n");
  else
    printf("Addr:%s\n", buf);
*/
  t.printf("220 %s ESMTP BHM\r\n", hostname);
  do_helo(t, buf, sizeof(buf));
  while( (rc = recv_msg(t, buf, sizeof(buf))) == 1)
    { }
  if(rc == eTimeout)
    t.printf("421 %s Error: timeout exceeded\r\n", hostname);
  if(rc < 0)
  {
    res.error();
    return;
  }
  if(rc == 0)
    t.sendCstr("221 2.0.0 Bye\r\n");
}

PVOID smtp_worker(PVOID param)
{
  thread_data *td = (thread_data *)param;
  do_work(td);
  if(td->debug)
    delete td->debug;
  thread_status[td->threadNum] = eFinished;
  close(td->fd);
  free(td);
  return 0;
}

int main(int argc, char **argv)
{
  int maxThreads = 100;
  bool allLog = false;
  PCCHAR debugName = NULL;
  bool debugMultipleFiles = false;
#ifdef USE_SSL
  int use_ssl = false;
#endif
  unsigned short port = 25;
#ifdef USE_GNUTLS
#if GNUTLS_VERSION_NUMBER <= 0x020b00
  gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
#endif
  gnutls_global_init();
  if(!gnutls_check_version(GNUTLS_VER))
  {
    fprintf(stderr, "Needs version " GNUTLS_VER " of GNUTLS\n");
    exit(1);
  }
#endif

  int c;
  while(-1 != (c = getopt(argc, argv, "m:p:st:z:Z:")) )
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
      case 'm':
        maxMsgSize = atoi(optarg);
      break;
      case 'p':
        port = atoi(optarg);
      break;
      case 's':
#ifdef USE_SSL
        use_ssl = true;
#else
        usage("SSL not supported.\n");
#endif
      break;
      case 't':
        maxThreads = atoi(optarg);
      break;
      case 'Z':
        debugMultipleFiles = true;
      case 'z':
        debugName = optarg;
      break;
    }
  }
  if(maxMsgSize < 0 || maxMsgSize > MAX_MSG_SIZE)
    usage("Bad maximum message size.\n");

  if(maxThreads < 0 || maxThreads > MAX_PROCESSES)
    usage("Bad maximum process limit.\n");

  if(optind + 1 != argc)
    usage();

  BHMUsers ul(argv[optind]);

/*  int fd[2];
  if(pipe(fd))
  {
    printf("Can't create pipe.\n");
    return eSystem;
  }*/
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

  log = new Logit("bhm.log", allLog, false, 0);
  Logit *debug = NULL;

  if(debugName)
    debug = new Logit(debugName, false, debugMultipleFiles, 0);

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in in;
  in.sin_family = AF_INET;
  in.sin_port = htons(port);
  in.sin_addr.s_addr = INADDR_ANY;
  if(listen_fd == -1 || bind(listen_fd, (sockaddr *)&in, sizeof(in))
   || listen(listen_fd, 10))
  {
    fprintf(stderr, "Can't bind to port.\n");
    return 1;
  }
  struct pollfd pfd;
  pfd.fd = listen_fd;
  pfd.events = POLLIN;
  pthread_t *thread_info = (pthread_t *)calloc(sizeof(thread_info), maxThreads);
  thread_status = (int *)calloc(sizeof(int), maxThreads);
  time_t nextPrint = time(NULL)/60*60+60;
  while(1)
  {
    time_t t = time(NULL);
    if(t >= nextPrint)
    {
      res.print();
      nextPrint += 60;
    }
    int timeout = (nextPrint - t) * 1000;
    int rc = poll(&pfd, 1, timeout);
    if(rc < 0)
    {
      if(errno == EINTR)
        continue;
      fprintf(stderr, "Poll error on accept.\n");
      return 1;
    }
    int fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if(rc == 1)
    {
      fd = accept(listen_fd, (sockaddr*)&addr, &addrlen);
      if(fd == -1)
      {
        res.error();
        continue;
      }
      res.connection();
      processes++;
      int i;
      for(i = 0; i < maxThreads && thread_status[i] == eUsed; i++)
        ;
      if(thread_status[i] == eFinished)
      {
        void *value_ptr;
        pthread_join(thread_info[i], &value_ptr);
        processes--;
      }

      thread_status[i] = eUsed;

      pthread_attr_t attr;
      if(pthread_attr_init(&attr)
       || pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)
       || pthread_attr_setstacksize(&attr, 32*1024))
        fprintf(stderr, "Can't set thread attributes.\n");
      thread_data *td = (thread_data *)malloc(sizeof(thread_data));
      td->threadNum = i;
      td->fd = fd;
      memcpy(&td->addr, &addr, sizeof(addr));
      td->debug = debug ? new Logit(*debug, i) : NULL;
#ifdef USE_SSL
      td->ssl = use_ssl;
#endif
      int p = pthread_create(&thread_info[i], &attr, smtp_worker, PVOID(td));
      pthread_attr_destroy(&attr);
      if(p)
      {
        fprintf(stderr, "Can't create a thread.\n");
        return 1;
      }
    }
  }
  return 0;
}

