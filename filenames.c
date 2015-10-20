#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "filenames.h"

void replace_percents(char* path, int path_len) {
  char buf[512];
  int pos = 0, bpos = 0;
  while (pos < path_len) {
    if (path[pos] != '%') {
      buf[bpos] = path[pos];
      bpos++;
      pos++;
    } else {
      if ((path_len - pos) >= 3 &&
          isxdigit(path[pos + 1]) &&
          isxdigit(path[pos + 2])) {
        char hex[3];
        hex[0] = path[pos + 1];
        hex[1] = path[pos + 2];
        hex[2] = '\0';

        int hexnum;
        sscanf(hex, "%x", &hexnum);

        buf[bpos] = (char) hexnum;
        pos += 3;
        bpos++;
      } else {
        buf[bpos] = path[pos];
        bpos++;
        pos++;
      }
    }
  }
  buf[bpos] = '\0';
  strcpy(path, buf);
}

void omit_query(char* path) {
  int i = 0;
  char c = path[i];
  while (c != '\0') {
    if (c == '?')
      c = path[i] = '\0';
    else
      c = path[++i];
  }
}
