#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct sysinfo info;

  if (sysinfo(&info) < 0) {
    fprintf(2, "sysinfo: failed\n");
    exit(1);
  }
  printf("free memory: %ld bytes (%ld pages)\n", info.freemem,
         info.freemem / 4096);
  printf("processes: %ld\n", info.nproc);
  exit(0);
}
