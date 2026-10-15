#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/uproc.h"
#include "user/user.h"

static struct uproc processes[NPROC];
static char *state_names[] = {
  "unused", "used", "sleeping", "runnable", "running", "zombie"
};

int
main(int argc, char *argv[])
{
  int count;
  int i;

  (void)argc;
  (void)argv;
  count = getprocs(processes, NPROC);
  if (count < 0) {
    fprintf(2, "ps: getprocs failed\n");
    exit(1);
  }

  printf("PID\tPPID\tSTATE\tSIZE\tNAME\n");
  for (i = 0; i < count; i++) {
    char *state = "unknown";

    if (processes[i].state >= 0 &&
        processes[i].state < (int)(sizeof(state_names) / sizeof(state_names[0])))
      state = state_names[processes[i].state];
    printf("%d\t%d\t%s\t%lu\t%s\n", processes[i].pid, processes[i].ppid,
           state, processes[i].sz, processes[i].name);
  }
  exit(0);
}