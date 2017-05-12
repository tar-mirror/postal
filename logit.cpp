#include "logit.h"

Logit::Logit(const char *filename, bool is_verbose)
 : m_sem(true)
 , m_verbose(is_verbose)
 , m_fp(fopen(filename, "w"))
{
  if(!m_fp)
  {
    fprintf(stderr, "Can't open file %s\n", filename);
    exit(1);
  }
}

Logit::~Logit()
{
  fclose(m_fp);
  fflush(NULL);
}

int Logit::Write(const char *data, size_t len)
{
  Lock l(m_sem);
  if(fwrite(data, 1, len, m_fp) != len)
    return -1;
  fflush(NULL);
  return 0;
}

