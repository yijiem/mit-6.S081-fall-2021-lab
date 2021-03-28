#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

/**
 * $ echo hello too | xargs echo bye
 * bye hello too
 *
 * $ printf '1\n2' | xargs echo line
 * line1
 * line2
 */

char whitespace[] = " ";

int gettoken(char** p1, char** p2) {
  char* s = *p2;
  while (*s != 0 && strchr(whitespace, *s)) {
    s++;
  }
  if (*s == 0) {
    return 0;
  }
  *p1 = s;
  while (*s != 0 && !strchr(whitespace, *s)) {
    s++;
  }
  *p2 = s;
  return 1;
}

int main(int argc, char *argv[]) {
  char* argvv[MAXARG];
  int p = 0;
  for (int i = 1; i < argc; ++i) {
    argvv[p++] = argv[i];
  }
  char line[100];
  char c;
  int i = 0;
  while (read(0, &c, 1) == 1) {
    if (c == '\n') {
      line[i] = 0;
      i = 0;
      int cp = p;
      char* eargvv[MAXARG];
      int ep = 0;
      char* p1 = line;
      char* p2 = line;
      while (gettoken(&p1, &p2)) {
        if (cp == MAXARG - 1) {
          fprintf(2, "xargs: too many tokens\n");
          exit(1);
        }
        argvv[cp++] = p1;
        eargvv[ep++] = p2;
      }
      // null terminate
      for (int j = 0; j < ep; ++j) {
        *eargvv[j] = 0;
      }
      argvv[cp] = 0;

      /*
      int d = 0;
      while (argvv[d] != 0) {
        printf("%s ", argvv[d++]);
      }
      printf("\n");
      */

      if (fork() == 0) {
        exec(argvv[0], argvv);
        fprintf(2, "xargs: exec failed\n");
        exit(1);
      }
      wait(0);
      continue;
    }
    if (i == 99) {
      fprintf(2, "xargs: line too long\n");
      exit(1);
    }
    line[i++] = c;
  }
  exit(0);
}
