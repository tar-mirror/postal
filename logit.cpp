#include "logit.h"
#include <limits.h>

Logit::Logit(const char *filename, bool is_verbose, bool numbered_files, int pid)
 : m_verbose(is_verbose)
 , m_fp(NULL)
 , m_count(0)
 , m_numbered_files(numbered_files)
 , m_filename(filename)
 , m_pid(pid)
{
}

Logit::Logit(const Logit &l, int pid)
 : m_verbose(l.m_verbose)
 , m_fp(NULL)
 , m_count(0)
 , m_numbered_files(l.m_numbered_files)
 , m_filename(l.m_filename)
 , m_pid(pid)
{
}

bool Logit::reopen()
{
  if(!m_numbered_files && m_fp)
    return false;
  char buf[PATH_MAX];

  if(m_fp)
  {
    fflush(m_fp);
    fclose(m_fp);
  }
  char pid[12], count[12];
  sprintf(pid, "%d", m_pid);
  sprintf(count, "%d", m_count);
  strncpy(buf, m_filename.c_str(), PATH_MAX - 25);
  buf[PATH_MAX - 25] = '\0';
  if(m_pid)
  {
    strcat(buf, ":");
    strcat(buf, pid);
  }
  if(m_numbered_files)
  {
    strcat(buf, ":");
    strcat(buf, count);
  }
  m_count++;
  m_fp = fopen(buf, "w");
  if(!m_fp)
  {
    fprintf(stderr, "Can't open file %s\n", buf);
    m_fp = fdopen(2, "w");
  }
  return false;
}

Logit::~Logit()
{
  if(m_fp)
  {
    fflush(m_fp);
    fclose(m_fp);
  }
}

int Logit::Write(const char *data, size_t len)
{
  if(!m_fp)
    reopen();
  if(fwrite(data, 1, len, m_fp) != len)
    return -1;
  fflush(m_fp);
  return 0;
}

