#ifndef LOGIT_H
#define LOGIT_H

#include "mutex.h"
#include <stdio.h>

#include <string>

class Logit
{
public:
  Logit(const char *filename, bool is_verbose);
  ~Logit();

  int Write(const char *data, size_t len);
  int Write(const string &str) { return Write(str.c_str(), str.size() ); }
  bool verbose() const { return m_verbose; }

private:
  Mutex m_sem;
  bool m_verbose;
  FILE *m_fp;

  Logit(const Logit&);
  Logit & operator=(const Logit&);
};

#endif

