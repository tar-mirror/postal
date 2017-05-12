#include "cmd5.h"
#include <stdio.h>

void Cmd5::init()
{
  MD5_Init(&m_context);
}

void Cmd5::getSum(char *buf)
{
  MD5_Final((unsigned char *)buf, &m_context);
}
  // get the sum into a string reference
string Cmd5::getSum()
{
  unsigned char output[16];
  getSum((char *)output);
  char str_output[33];
  for(int i = 0; i < 16; i++)
  {
    sprintf(&str_output[2 * i], "%02x", (int)output[i]);
  }
  return str_output;
}

void Cmd5::addData(const char *buf, size_t bytes)
{
  MD5_Update(&m_context, (const unsigned char *)buf, bytes);
}

void Cmd5::addData(const string &buf)
{
  addData((const char *)buf.c_str(), buf.size());
}

