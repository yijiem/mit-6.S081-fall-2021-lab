#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

/*
 * Sample:
 * yijiem-macpro:Desktop yijiem$ find ~/test -name a
 * /Users/yijiem/test/a
 * /Users/yijiem/test/a/a
 * /Users/yijiem/test/a/b/a
 * /Users/yijiem/test/a/b/a/a
 * /Users/yijiem/test/a/b/a/b/a
 */

void find_routine(const char* path, const char* filename) {
  int fd;
  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    exit(1);
  }
  char buf[512];
  char* p;
  struct dirent de;
  struct stat st;
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    // printf("<dirent> path: %s, inum: %d, name: %s\n", path, de.inum, de.name);
    if (de.inum == 0) {
      continue;
    }
    if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
      continue;
    }
    // Construct path
    if (strlen(path) + 2 + DIRSIZ + 1 > sizeof(buf)) {
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if (stat(buf, &st) < 0) {
      printf("find: cannot stat %s\n", buf);
      continue;
    }
    if (strcmp(de.name, filename) == 0) {
      // Found
      printf("%s\n", buf);
    }
    if (st.type == T_DIR) {
      find_routine(buf, filename);
    }
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(2, "Usage: %s <dir> <name>\n", argv[0]);
    exit(1);
  }
  char* path = argv[1];
  char* filename = argv[2];
  struct stat st;
  if (stat(path, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    exit(1);
  }
  if (st.type != T_DIR) {
    fprintf(2, "find: %s is not a directory\n", path);
    exit(1);
  }
  find_routine(path, filename);
  exit(0);
}
