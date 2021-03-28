#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: %s <number of ticks>\n", argv[0]);
    exit(1);
  }
  int n = atoi(argv[1]);
  if (sleep(n) != 0) {
    printf("sleep failed\n");
    exit(1);
  }
  exit(0);
}
