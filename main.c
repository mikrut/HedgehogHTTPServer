#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>


#define DBG_INT(var) printf(#var" = %d\n", (var))
#define DBG_STR(str) puts((str))
#define DBG_ADR(adr) printf(#adr" = %p\n", (adr))

void write_response(int sockfd,
                    int status_code, const char* status_string,
                    const char* content_type,
                    const char* buf, size_t buflen) {
  const char* response_header_template = "\
HTTP/1.1 %d %s\n\
Server: HedgehogHTTPServer\n\
Content-Language: en, ru\n\
Content-Length: %d\n\
Content-Type: %s\n\
Connection: close\n\
\n";
  int header_size = strlen(response_header_template) + 4 + 10 + strlen(content_type);
  char* header = (char*) malloc(sizeof(char) * header_size);
  sprintf(header, response_header_template, status_code, status_string,
          buflen, content_type);
  int header_true_len = strlen(header);

  char* response = (char*) malloc(sizeof(char) * (header_true_len + buflen + 1));
  memcpy(response, header, header_true_len);
  memcpy(response + header_true_len, buf, buflen);
  response[header_true_len + buflen + 1] = '\0';

  puts(response);
  write(sockfd, response, header_true_len + buflen + 1);//, MSG_CONFIRM);
}

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

int main(void) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DBG_INT(sockfd);

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
  setnonblocking(sockfd);
  listen(sockfd, 5);

  const int MAX_EVENTS = 10;
  struct epoll_event ev, events[MAX_EVENTS];
  int epollfd, nfds;

  epollfd = epoll_create1(0);
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = sockfd;
  DBG_INT(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev));

  char buf[256];
  memset(buf, 256, 0);

  for(;;) {
    DBG_STR("waiting");
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    DBG_STR("waited");

    int n;
    for (n = 0; n < nfds; n++) {
      if (events[n].data.fd == sockfd) {
        while(1) {
          DBG_STR("if");
          struct sockaddr_in client;
          int addrlen = sizeof(client);
          int acc = accept(sockfd, (struct sockaddr*) &client, (socklen_t*) &addrlen);
          if (-1 == acc) {
            if ((errno == EAGAIN) ||
                (errno == EWOULDBLOCK)) {
              DBG_INT(errno);
              DBG_INT(EAGAIN);
              DBG_INT(EWOULDBLOCK);
              break;
            } else {
              perror ("accept");
              break;
            }
          }

          DBG_INT(acc);

          DBG_INT(setnonblocking(acc));
          ev.events = EPOLLIN | EPOLLET;
          ev.data.fd = acc;
          DBG_INT(epoll_ctl(epollfd, EPOLL_CTL_ADD, acc, &ev));
        }
      } else {
        DBG_STR("else");
        DBG_INT(events[n].data.fd);
        DBG_ADR(buf);
        DBG_ADR(&events[n]);

        int input_len = read(events[n].data.fd, buf, 245);
        puts(buf);

        const char* resp = "Hello, world!";
        write_response(events[n].data.fd, 200, "OK", "text/html", resp, strlen(resp));
        close(events[n].data.fd);
      }
    }
  }

  return 0;
}
