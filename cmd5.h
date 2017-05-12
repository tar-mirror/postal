extern "C"
{
#include <openssl/md5.h>
}

#include <string>

class Cmd5
{
public:
  Cmd5() { init(); }

  void init();

  // get the sum as a 16byte buffer
  void getSum(char *buf);
  // get the sum into a string reference
  string getSum();

  // add some more data to the data that has been summed.
  void addData(const char *buf, size_t bytes);
  void addData(const string &buf);

private:
  MD5_CTX m_context;

};
