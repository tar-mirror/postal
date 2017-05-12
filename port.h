#ifndef PORT_H
#define PORT_H




#ifndef _LARGEFILE64_SOURCE

#endif
#ifdef _LARGEFILE64_SOURCE
#define OFF_T_PRINTF "%lld"
#else
#define OFF_T_PRINTF "%d"
#endif

#if 0
#define false 0
#define true 1
#endif

typedef struct timeval TIMEVAL_TYPE;

#ifdef _LARGEFILE64_SOURCE
#define OFF_TYPE off64_t
#define file_lseek lseek64
#define file_creat creat64
#define file_open open64
#else
#define OFF_TYPE off_t
#define file_lseek lseek
#define file_creat creat
#define file_open open
#endif

typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef const char * PCCHAR;
typedef char * PCHAR;
typedef PCHAR const CPCHAR;
typedef PCCHAR const CPCCHAR;
typedef void * PVOID;
typedef PVOID const CPVOID;
typedef const CPVOID CPCVOID;

#endif
