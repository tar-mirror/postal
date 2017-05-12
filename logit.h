#ifndef LOGIT_H
#define LOGIT_H

using namespace std;

#include <stdio.h>

#include <string>

class Logit
{
public:
  Logit(const char *filename, bool is_verbose, bool numbered_files, int pid);
  ~Logit();

  int Write(const char *data, size_t len);
  int Write(const string &str) { return Write(str.c_str(), str.size() ); }
  bool verbose() const { return m_verbose; }
  bool reopen();

  Logit(const Logit &l, int pid);
private:
  bool m_verbose;
  FILE *m_fp;
  int m_count;
  int m_numbered_files;
  string m_filename;
  int m_pid;

  Logit(const Logit&);
  Logit & operator=(const Logit&);
};

#endif

