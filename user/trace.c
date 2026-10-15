#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

static void
usage(void)
{
  fprintf(2, "usage: trace <mask> <cmd> [args...]\n");
  exit(1);
}

static int
parse_mask(const char *text, int *mask)
{
  uint value = 0;
  int negative = 0;

  if (*text == '-' || *text == '+') {
    negative = *text == '-';
    text++;
  }
  if (*text == '\0')
    return -1;
  for (; *text; text++) {
    uint digit;
    uint limit = negative ? 0x80000000U : 0x7fffffffU;

    if (*text < '0' || *text > '9')
      return -1;
    digit = *text - '0';
    if (value > (limit - digit) / 10)
      return -1;
    value = value * 10 + digit;
  }
  if (negative && value == 0x80000000U)
    *mask = -2147483647 - 1;
  else if (negative)
    *mask = -(int)value;
  else
    *mask = (int)value;
  return 0;
}

int
main(int argc, char *argv[])
{
  int mask;

  if (argc < 3 || parse_mask(argv[1], &mask) < 0)
    usage();
  if (trace(mask) < 0)
    exit(1);
  exec(argv[2], &argv[2]);
  fprintf(2, "trace: exec %s failed\n", argv[2]);
  exit(1);
}