#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char path[512];
int shownum;

void
find(char *name)
{
  int fd, len;
  char *p;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (st.type != T_DIR) {
    fprintf(2, "find: %s is not a directory\n", path);
    close(fd);
    return;
  }

  len = strlen(path);
  if (len + 1 + DIRSIZ + 1 > sizeof path) {
    fprintf(2, "find: path too long\n");
    close(fd);
    return;
  }
  path[len] = '/';
  p = path + len + 1;
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
      continue;
    if (stat(path, &st) < 0) {
      fprintf(2, "find: cannot stat %s\n", path);
      continue;
    }
    if (strcmp(p, name) == 0) {
      if (shownum)
        printf("%s %d\n", path, st.ino);
      else
        printf("%s\n", path);
    }
    if (st.type == T_DIR)
      find(name);
  }
  path[len] = 0;
  close(fd);
}

int
main(int argc, char *argv[])
{
  if (argc == 4 && strcmp(argv[3], "-n") == 0)
    shownum = 1;
  else if (argc != 3) {
    fprintf(2, "usage: find dir name [-n]\n");
    exit(1);
  }
  if (strlen(argv[1]) >= sizeof path) {
    fprintf(2, "find: path too long\n");
    exit(1);
  }
  strcpy(path, argv[1]);
  find(argv[2]);
  exit(0);
}
