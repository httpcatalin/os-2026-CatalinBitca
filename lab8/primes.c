#include "kernel/types.h"
#include "user/user.h"

void
sieve(int left)
{
  int p, n;

  if(read(left, &p, sizeof(p)) != sizeof(p)){
    close(left);
    return;
  }
  printf("prime %d\n", p);

  int right[2];
  pipe(right);
  if(fork() == 0){
    close(right[1]);
    close(left);
    sieve(right[0]);
    exit(0);
  }
  close(right[0]);
  while(read(left, &n, sizeof(n)) == sizeof(n)){
    if(n % p != 0)
      write(right[1], &n, sizeof(n));
  }
  close(left);
  close(right[1]);
  wait(0);
}

int
main(int argc, char *argv[])
{
  int p[2];

  pipe(p);
  if(fork() == 0){
    close(p[1]);
    sieve(p[0]);
    exit(0);
  }
  close(p[0]);
  for(int i = 2; i <= 35; i++)
    write(p[1], &i, sizeof(i));
  close(p[1]);
  wait(0);
  exit(0);
}
