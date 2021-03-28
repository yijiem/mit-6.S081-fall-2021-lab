#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
  int p[2];
  pipe(p);
  if (fork() == 0) {
    int pid = getpid();
    char b[512];
    read(p[0], b, 512);
    printf("%d: received ping\n", pid);
    close(p[0]);
    write(p[1], b, 512);
    exit(0);
  }
  int pid = getpid();
  char b[512];
  write(p[1], b, 512);
  char bb;
  write(p[1], &bb, 1);
  close(p[1]);
  read(p[0], &bb, 1);
  read(p[0], &b, 512);
  printf("%d: received pong\n", pid);
  wait(0);
  exit(0);
}
