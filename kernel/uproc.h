#include "types.h"

struct uproc {
  int pid;
  int ppid;
  int state;
  uint64 sz;
  char name[16];
};