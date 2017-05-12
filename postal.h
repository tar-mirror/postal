#ifndef POSTAL_H
#define POSTAL_H

typedef enum
{
  eWONT,
  eMUST,
  eNONE
} TRISTATE;

#define VER_STR "0.60"
#define USE_SSL
#define LINUX_PTHREAD


#if 0
#define false 0
#define true 1
#endif

#include <stdio.h>

#define MAX_PROCESSES 200
#define MAX_MSG_SIZE 40960
// The amount of time between updates to the shared memory region for random
// data.
#define RAND_TIME 5
// The allowable amount of time for results to lag behind the actions that
// generated them.  Should be a small fraction of a minute.  The smaller it
// is the more IPC there is.
#define RESULTS_LAG 5
#define MAX_BYTES_UNREPORTED 40960
#define SEM_KEY 4710

#define CHECK_PERCENT(XX) ((XX) == 100 || rand() % 100 < (XX))

typedef const char * PCCHAR;
typedef char * PCHAR;
typedef PCHAR const CPCHAR;
typedef PCCHAR const CPCCHAR;
typedef void * PVOID;
typedef PVOID const CPVOID;
typedef const CPVOID CPCVOID;

typedef enum
{
  eParam = 1,
  eSystem = 2,
  eCtrl_C = 5,
  eNoError = 0,
  eSocket = -1,
  eCorrupt = -2,
  eTimeout = -3,
  eServer = -4
} ERROR_TYPE;

#endif
