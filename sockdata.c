#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "sockdata.h"

void sockdata_initarr(struct send_data sockdata[], int arr_len) {
  for (int i = 0; i < arr_len; i++) {
    sockdata[i].sockfd = -1;
    sockdata[i].fd = -1;
    sockdata[i].buf = NULL;
    sockdata[i].blen = sockdata[i].bpos = sockdata[i].bsize = 0;
  }
}

void sockdata_init(struct send_data* sockdata, int fd, int sockfd) {
  sockdata->bsize = 1024;
  sockdata->buf = malloc(sizeof(char) * sockdata->bsize);
  sockdata->blen = sockdata->bpos = 0;
  sockdata->fd = fd;
  sockdata->sockfd = sockfd;
}

void sockdata_fin(struct send_data* sockdata) {
  if (sockdata->fd != -1) {
    close(sockdata->fd);
    sockdata->fd = -1;
  }
  if (sockdata->sockfd != -1) {
    close(sockdata->sockfd);
    sockdata->sockfd = -1;
  }
  if (sockdata->buf != NULL) {
    free(sockdata->buf);
    sockdata->buf = NULL;
  }
  sockdata->blen = sockdata->bsize = sockdata->bpos = 0;
}

void read_data(struct send_data* sockdata) {
  if (sockdata->blen == sockdata->bpos && sockdata->fd != -1) {
    sockdata->blen = read(sockdata->fd, sockdata->buf, sockdata->bsize);
    sockdata->bpos = 0;
    if (sockdata->blen == 0) {
      close(sockdata->fd);
      sockdata->fd = -1;
    }
  }
}

bool find_sockdata(int sockfd, struct send_data sockdata[], int arrlen,
                  struct send_data** out) {
  struct send_data* found = NULL;
  struct send_data* fempty = NULL;
  for (int i = 0; i < arrlen && found == NULL; i++) {
    if (fempty == NULL && sockdata[i].sockfd == -1)
      fempty = &(sockdata[i]);
    if (sockdata[i].sockfd == sockfd)
      found = &(sockdata[i]);
  }

  if (found != NULL)
    (*out) = found;
  else
    (*out) = fempty;

  return found != NULL;
}

void sockdata_set_string(struct send_data* sockdata, const char* str) {
  strncpy(sockdata->buf, str, sockdata->bsize);
  int len = strnlen(str, sockdata->bsize);
  sockdata->blen = len;
  sockdata->bpos = 0;
}

void sockdata_append(struct send_data* sockdata, const char* str) {
  int left = sockdata->bsize - sockdata->blen;
  int len = strnlen(str, left);
  strncat(sockdata->buf, str, left);
  sockdata->blen += len;
}

void continue_transmit(struct send_data* sockdata) {
  int written = 0;
  printf("continue transmit fd=%d sockfd=%d\n", sockdata->fd, sockdata->sockfd);

  do {
    read_data(sockdata);
    int left_size = sockdata->blen - sockdata->bpos;

    while (left_size && written >= 0) {
      char* wrtpos = sockdata->buf + sockdata->bpos;
      written = send(sockdata->sockfd, wrtpos, left_size, MSG_NOSIGNAL);

      if (written >= 0) {
        sockdata->bpos += written;
        left_size = sockdata->blen - sockdata->bpos;
      }
    }
  } while ((sockdata->fd != -1 || sockdata->bpos != sockdata->blen) &&
    written >= 0);

  if ((sockdata->fd == -1) && (sockdata->bpos == sockdata->blen)) {
    puts("End of transmission");
    sockdata_fin(sockdata);
  } else {
    switch(errno) {
      case EPIPE:
        puts("EPIPE"); break;
      case EAGAIN:
        puts("EAGAIN"); break;
      default:
        puts("Unknown error on write"); break;
    }
    sockdata_fin(sockdata);
  }

}
