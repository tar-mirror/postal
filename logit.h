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
  FILE *m_fp;
  bool m_verbose;
};

#endif

