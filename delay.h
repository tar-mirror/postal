

typedef struct
{
  char operation; // 'R' or 'W' for read or write
  int bytes; // number of bytes to read/write
  unsigned int ip; // ip address
} Sdelay;

// probability of a particular speed is s[i].prob / (sum s[0..n].prob)
typedef struct
{
  int speed; // bps
  int prob; // number of systems with this speed.
} Sspeed;

// must end with 0,0
Sspeed *speeds = {
{ 28800, 10 },
{ 33600, 15 }, // 33K6 or 56K modem
{ 64000, 5 }, // ISDN 1*B
{ 128000, 10 }, // ISDN 2*B
{ 256000, 5 }, // slow frame
{ 1536000, 30 }, // T1
{ 2048000, 25 }, // E1
{ 0, 0 } };

class delay
{
public:
  
