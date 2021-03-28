#include "kernel/types.h"
#include "user/user.h"

void child_routine(int* p) {
  int n;
  close(p[1]);
  read(p[0], &n, 4);
  printf("prime %d\n", n);
  int nn;
  int pp[2];
  int forked = 0;
  while (read(p[0], &nn, 4) > 0) {
    if (nn % n == 0) {
      continue;
    }
    if (!forked) {
      pipe(pp);
      if (fork() == 0) {
        child_routine(pp);
      }
      forked = 1;
      close(pp[0]);
    }
    // pass to the right neighbour
    write(pp[1], &nn, 4);
  }
  close(pp[1]);
  if (!forked) {
    exit(0);
  }
  wait(0);
  exit(0);
}

int main(int argc, char* argv[]) {
  int p[2];
  pipe(p);
  if (fork() == 0) {
    child_routine(p);
  }
  close(p[0]);
  for (int i = 2; i < 36; ++i) {
    write(p[1], &i, 4);
  }
  close(p[1]);
  wait(0);
  exit(0);
}
