#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <libgen.h>


#define DBG_INT(var) printf(#var" = %d\n", (var))
#define DBG_STR(str) puts((str))
#define DBG_ADR(adr) printf(#adr" = %p\n", (adr))

const int ext_cmp_len = 5;

int in(const char** array, int arr_len, const char* str) {
  for (int i = 0; i < arr_len; i++)
    if (!strncmp(array[i], str, ext_cmp_len))
      return true;
  return false;
}

void ext_to_mime(const char* ext, char* result) {
  const char* images[] = {"jpg", "jpeg", "png", "gif"};
  const char* texts[] = {"html", "css"};

  if (ext != NULL) {
    if (in(images, sizeof(images)/sizeof(images[0]), ext)) {
      strncpy(result, "image/", 20);
      strncat(result, ext, ext_cmp_len);
    } else if (in(texts, sizeof(texts)/sizeof(texts[0]), ext)) {
      strncpy(result, "text/", 6);
      strncat(result, ext, ext_cmp_len);
    } else if (!strncmp(ext, "swf", ext_cmp_len)) {
      const char swf_mime[] = "application/vnd.adobe.flash-movie";
      strncpy(result, swf_mime, sizeof(swf_mime));
    } else if (!strncmp(ext, "js", ext_cmp_len)){
      const char js_mime[] = "application/javascript";
      strncpy(result, js_mime, sizeof(js_mime));
    } else {
      const char plain_mime[] = "text/plain";
      strncpy(result, plain_mime, sizeof(plain_mime));
    }
  } else {
    const char plain_mime[] = "text/plain";
    strncpy(result, plain_mime, sizeof(plain_mime));
  }
}

void write_header(int sockfd,
                 int status_code, const char* status_string,
                 int content_length, const char* content_type) {
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
          content_length, content_type);
  int header_true_len = strlen(header);

  puts(header);
  write(sockfd, header, header_true_len);
}

void write_response(int sockfd,
                    int status_code, const char* status_string,
                    const char* content_type,
                    const char* buf, size_t buflen) {
  write_header(sockfd, status_code, status_string, buflen, content_type);

  puts(buf);
  write(sockfd, buf, buflen);
}

void write_file_response(char* file_path, int sockfd) {
  char* name = basename(file_path);
  const char* ext = strrchr(name, '.');
  char mime_buf[64];
  if ((ext != NULL)) {
    ext_to_mime(ext+1, mime_buf);
  }

  int fd = open(file_path, O_RDONLY);
  if (fd != -1) {
    struct stat file_stat;
    fstat(fd, &file_stat);
    int file_len = file_stat.st_size;

    write_header(sockfd, 200, "OK", file_len, mime_buf);

    const int READ_BUF_SIZE = 1024;
    char read_buf[READ_BUF_SIZE];
    int read_size, left_size;

    do {
      left_size = read_size = read(fd, read_buf, READ_BUF_SIZE);
      char* p = read_buf;

      while(left_size) {
        int written = write(sockfd, read_buf, read_size);
        left_size -= written;
        p += written;
      }
    } while(read_size > 0);

  } else {
    const char not_found_message[] =
      "Requested file was not found on this server";
    write_response(sockfd,
      404, "Not Found",
      "text/plain",
      not_found_message, sizeof(not_found_message));
  }
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
        if (input_len > 0) {
          puts(buf);

          write_file_response("index.html", events[n].data.fd);
          //const char* resp = "Hello, world!";
          //write_response(events[n].data.fd, 200, "OK", "text/html", resp, strlen(resp));
          close(events[n].data.fd);
        }
      }
    }
  }

  return 0;
}
