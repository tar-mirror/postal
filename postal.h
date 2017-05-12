#ifndef POSTAL_H
#define POSTAL_H

typedef enum
{
  eWONT,
  eMUST,
  eNONE
} TRISTATE;

#define VER_STR "0.51"
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

#endif