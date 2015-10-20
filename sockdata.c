#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>
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
    sockdata[i].waited = false;
  }
}

void sockdata_init(struct send_data* sockdata, int fd, int sockfd) {
  sockdata->bsize = 1024;
  sockdata->buf = malloc(sizeof(char) * sockdata->bsize);
  sockdata->blen = sockdata->bpos = 0;
  sockdata->fd = fd;
  sockdata->sockfd = sockfd;
  sockdata->waited = false;
}

/*** THIS CODE IS FROM stackoverflow.com ***/

int getSO_ERROR(int fd) {
   int err = 1;
   socklen_t len = sizeof err;
   if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len))
      perror("getSO_ERROR");
   if (err)
      errno = err;              // set errno to the socket SO_ERROR
   return err;
}

void closeSocket(int fd) {      // *not* the Windows closesocket()
   if (fd >= 0) {
      getSO_ERROR(fd); // first clear any errors, which can cause close to fail
      if (shutdown(fd, SHUT_RDWR) < 0) // secondly, terminate the 'reliable' delivery
         if (errno != ENOTCONN && errno != EINVAL) // SGI causes EINVAL
            perror("shutdown");
      if (close(fd) < 0) // finally call close()
         perror("close");
   }
}

bool haveInput(int fd, double timeout) {
   int status;
   fd_set fds;
   struct timeval tv;
   FD_ZERO(&fds);
   FD_SET(fd, &fds);
   tv.tv_sec  = (long)timeout; // cast needed for C++
   tv.tv_usec = (long)((timeout - tv.tv_sec) * 1000000); // 'suseconds_t'

   while (1) {
      if (!(status = select(fd + 1, &fds, 0, 0, &tv)))
         return true;
      else if (status > 0 && FD_ISSET(fd, &fds))
         return false;
      else if (status > 0) {
         perror("I am confused");
         return false;
      } else if (errno != EINTR) {
         perror("select"); // tbd EBADF: man page "an error has occurred"
         return false;
      }
   }
}

bool flushSocketBeforeClose(int fd, double timeout) {
   const double start = time(NULL);
   char discard[99];

   if (shutdown(fd, 1) != -1)
      while (difftime(time(NULL), start) < timeout)
         while (haveInput(fd, 0.01)) // can block for 0.01 secs
            if (!read(fd, discard, sizeof discard))
               return true; // success!
   return false;
}

/*** END OF stackoverflow.com CODE ***/

void sockdata_fin(struct send_data* sockdata, int *snd_waitors) {
  printf("finalize socket=%d fd=%d\n", sockdata->sockfd, sockdata->fd);
  if (sockdata->fd != -1) {
    close(sockdata->fd);
    sockdata->fd = -1;
  }
  if (sockdata->sockfd != -1) {
    flushSocketBeforeClose(sockdata->sockfd, 0.05);
    closeSocket(sockdata->sockfd);
    sockdata->sockfd = -1;
  }
  if (sockdata->buf != NULL) {
    free(sockdata->buf);
    sockdata->buf = NULL;
  }
  sockdata->blen = sockdata->bsize = sockdata->bpos = 0;
  if (sockdata->waited)
    (*snd_waitors)--;
  sockdata->waited = false;
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

void continue_transmit(struct send_data* sockdata, int *snd_waitors) {
  int written = 0;
  int steps_count = 0;
  const int max_steps = 5;

  do {
    read_data(sockdata);
    int left_size = sockdata->blen - sockdata->bpos;

    while (left_size && written >= 0 && steps_count <= max_steps) {
      char* wrtpos = sockdata->buf + sockdata->bpos;
      written = send(sockdata->sockfd, wrtpos, left_size, MSG_NOSIGNAL);
      steps_count++;


      if (written >= 0) {
        sockdata->bpos += written;
        left_size = sockdata->blen - sockdata->bpos;
      }
    }
  } while ((sockdata->fd != -1 || sockdata->bpos != sockdata->blen) &&
    written >= 0 && steps_count <= max_steps);

  if ((sockdata->fd == -1) && (sockdata->bpos == sockdata->blen)) {
    sockdata_fin(sockdata, snd_waitors);
  } else if (written == -1) {
    switch(errno) {
      case EAGAIN: case EINTR:
        puts("EAGAIN or EINTR");break;
      case EPIPE:
        puts("EPIPE");
      default:
        puts("Error on write");
        sockdata_fin(sockdata, snd_waitors);
    }
  } else {
    if (!sockdata->waited) {
      (*snd_waitors)++;
      sockdata->waited = true;
    }
  }

}
