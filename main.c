#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "initvals.h"
#include "sockdata.h"
#include "dispatch.h"

int setnonblocking(int fd) {
  int flags;

#if defined(O_NONBLOCK)
  if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
    flags = 0;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
  flags = 1;
  return ioctl(fd, FIOBIO, &flags);
#endif
}

int main(int argc, char** argv) {
  struct initvals inits;
  init_params(argc, argv, &inits);
  print_initial(&inits);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("Could not create a socket");
    exit(errno);
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(inits.port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd,
           (struct sockaddr*) &server_addr,
           sizeof(server_addr)) == -1) {
    perror("Could not bind a socket");
    exit(errno);
  }
  setnonblocking(sockfd);
  if(listen(sockfd, 5) == -1) {
    perror("Could not listen a socket");
    exit(errno);
  }

  for (int i = 1; i < inits.nCPU; i++) {
    int pid = fork();
    if (pid == 0)
      break;
  }

  const int MAX_EVENTS = 10;
  struct epoll_event ev, events[MAX_EVENTS];
  int epollfd, nfds;

  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    perror("Could not create epollfd");
    exit(errno);
  }
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = sockfd;
  if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
    perror("Could not add socket to epoll");
    exit(errno);
  }

  struct send_data snd_arr[128];
  int snd_arr_waiting = 0;
  sockdata_initarr(snd_arr, 128);

  for(;;) {
    int timeout = -1;
    if (snd_arr_waiting > 0) {
      timeout = 500 - snd_arr_waiting * 45;
      if (timeout < 0)
        timeout = 100;
    }
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, timeout);

    int n;
    for (n = 0; n < nfds; n++) {
      if (events[n].data.fd == sockfd) {
        while(1) {
          struct sockaddr_in client;
          int addrlen = sizeof(client);
          int acc = accept(sockfd, (struct sockaddr*) &client, (socklen_t*) &addrlen);

          if (-1 == acc) {
            if ((errno == EAGAIN) ||
                (errno == EWOULDBLOCK)) {
              break;
            } else {
              perror ("accept");
              break;
            }
          }

          setnonblocking(acc);
          ev.events = EPOLLIN | EPOLLET;
          ev.data.fd = acc;
          epoll_ctl(epollfd, EPOLL_CTL_ADD, acc, &ev);
        }
      } else {
        dispatch_request(events[n].data.fd, &inits, snd_arr, 128, &snd_arr_waiting);
      }
    }

    for (int i = 0; i < 128; i++) {
      if (snd_arr[i].sockfd > 0) {
        continue_transmit(&snd_arr[i], &snd_arr_waiting);
      }
    }
  }

  return 0;
}
