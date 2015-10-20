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
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <libgen.h>


#define DBG_INT(var) printf(#var" = %d\n", (var))
#define DBG_STR(str) puts((str))
#define DBG_ADR(adr) printf(#adr" = %p\n", (adr))

struct send_data {
  int sockfd;
  int fd;
  char* buf;
  int bsize;
  int bpos;
  int blen;
};

const int ext_cmp_len = 5;

int in(const char** array, int arr_len, const char* str) {
  for (int i = 0; i < arr_len; i++)
    if (!strncmp(array[i], str, ext_cmp_len))
      return true;
  return false;
}

void ext_to_mime(const char* _ext, char* result) {
  char ext[10] = "";
  strncpy(ext, _ext, 8);
  if (!strcmp(ext, "jpg"))
    strcpy(ext, "jpeg");

  const char* images[] = {"jpeg", "png", "gif"};
  const char* texts[] = {"html", "css"};

  if (ext != NULL) {
    if (in(images, sizeof(images)/sizeof(images[0]), ext)) {
      strncpy(result, "image/", 20);
      strncat(result, ext, ext_cmp_len);
    } else if (in(texts, sizeof(texts)/sizeof(texts[0]), ext)) {
      strncpy(result, "text/", 6);
      strncat(result, ext, ext_cmp_len);
    } else if (!strncmp(ext, "swf", ext_cmp_len)) {
      const char swf_mime[] = "application/x-shockwave-flash";
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
  char response_header_template[] = "\
HTTP/1.1 %d %s\r\n\
Server: Hedgehog HTTP Server\r\n\
Content-Language: en, ru\r\n\
Content-Length: %d\r\n\
Content-Type: %s\r\n\
Connection: close\r\n\
\r\n";
  int header_size = strlen(response_header_template) + 4 + 10 + strlen(content_type);
  char* header = (char*) malloc(sizeof(char) * header_size);
  sprintf(header, response_header_template, status_code, status_string,
          content_length, content_type);
  int header_true_len = strlen(header);

  write(sockfd, header, header_true_len);
  free(header);
}

void write_response(int sockfd,
                    int status_code, const char* status_string,
                    const char* content_type,
                    const char* buf, size_t buflen) {
  write_header(sockfd, status_code, status_string, buflen, content_type);
  write(sockfd, buf, buflen);
}

void write_400(int sockfd) {
  const char bad_request_message[] =
      "Bad request. Try something else.";
  write_response(sockfd,
    400, "Bad request",
    "text/plain",
    bad_request_message, sizeof(bad_request_message));
}

void write_403(int sockfd) {
  const char forbidden_message[] =
      "Access to this directory is forbidden!";
  write_response(sockfd,
    403, "Forbidden",
    "text/plain",
    forbidden_message, sizeof(forbidden_message));
}

void write_404(int sockfd) {
  const char not_found_message[] =
      "Requested file was not found on this server";
  write_response(sockfd,
    404, "Not Found",
    "text/plain",
    not_found_message, sizeof(not_found_message));
}

void write_501(int sockfd) {
 const char not_implemented_message[] =
      "This method is not implemented or is unknown for server";
  write_response(sockfd,
    501, "Not Implemented",
    "text/plain",
    not_implemented_message, sizeof(not_implemented_message));
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
  sockdata->sockfd = -1;
  free(sockdata->buf);
  sockdata->blen = sockdata->bsize = sockdata->bpos = 0;
}

void read_data(struct send_data* sockdata) {
  if (sockdata->blen == 0 && sockdata->fd != -1) {
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

void continue_transmit(struct send_data* sockdata) {
  int written = 0;

  do {
    read_data(sockdata);
    int left_size = sockdata->blen - sockdata->bpos;

    while (left_size && written >= 0) {
      char* wrtpos = sockdata->buf + sockdata->bpos;

      written = write(sockdata->sockfd, wrtpos, left_size);
      if (written >= 0) {
        sockdata->bpos += written;
        left_size = sockdata->blen - sockdata->bpos;
      }
    }
  } while ((sockdata->fd != -1 || sockdata->bpos != sockdata->blen) &&
    written >= 0);

  if (sockdata->fd == -1 && sockdata->bpos == sockdata->blen) {
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
  }

}

int getfd_for_path(char* req_path, struct stat* file_stat, bool* is_dir) {
  int fd = open(req_path, O_RDONLY);

  if (fd != -1) {
    fstat(fd, file_stat);
    if (S_ISDIR (file_stat->st_mode)) {
      (*is_dir) = true;
      strcat(req_path, "/index.html");
      fd = open(req_path, O_RDONLY);
      fstat(fd, file_stat);
    }
  }
  return fd;
}

void get_mime_for_path(char* file_path, char* mime_buf) {
  char* name = basename(file_path);
  const char* ext = strrchr(name, '.');

  if ((ext != NULL)) {
    ext_to_mime(ext+1, mime_buf);
  }
}

void write_file_response(char* req_path, int sockfd) {
  struct stat file_stat;
  bool is_dir = false;
  int fd = getfd_for_path(req_path, &file_stat, &is_dir);
  puts(req_path);

  if (fd != -1) {
    char mime[64];
    int file_len = file_stat.st_size;
    get_mime_for_path(req_path, mime);
    write_header(sockfd, 200, "OK", file_len, mime);

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

    close(fd);
  } else {
    if (is_dir)
      write_403(sockfd);
    else
      write_404(sockfd);
  }
}

void write_file_info(char* req_path, int sockfd) {
  struct stat file_stat;
  bool is_dir = false;
  int fd = getfd_for_path(req_path, &file_stat, &is_dir);
  close(fd);
  puts(req_path);

  if (fd != -1) {
    char mime[64];
    int file_len = file_stat.st_size;
    get_mime_for_path(req_path, mime);

    const char* ok_string = "OK";
    write_header(sockfd, 200, ok_string, file_len, mime);
  } else {
    const char* not_found = "Not Found";
    write_header(sockfd, 404, not_found, 0, "");
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

struct initvals {
  int nCPU;
  char root_dir[256];
  int port;
};

const struct initvals DEFAULT_INITIALS = {
  4, "/home/mihanik/Desktop/Hedgehog HTTP Server/", 8080
};

void dispatch_GET(const char* request, int req_len, int sockfd,
  struct initvals* inits);
void dispatch_POST(const char* request, int req_len, int sockfd,
  struct initvals* inits);
void dispatch_HEAD(const char* request, int req_len, int sockfd,
  struct initvals* inits);
void dispatch_other(const char* request, int req_len, int sockfd,
  struct initvals* inits);

void dispatch_request(const char* request, int req_len, int sockfd,
  struct initvals* inits){
  char req_type[8];
  sscanf(request, "%s", req_type);
  if (!strcmp("GET", req_type))
    dispatch_GET(request, req_len, sockfd, inits);
  else if (!strcmp("POST", req_type))
    dispatch_POST(request, req_len, sockfd, inits);
  else if (!strcmp("HEAD", req_type))
    dispatch_HEAD(request, req_len, sockfd, inits);
  else
    dispatch_other(request, req_len, sockfd, inits);
}

void dispatch_GET(const char* request, int req_len, int sockfd,
  struct initvals* inits) {
  if (req_len > 0) {
    char req_path[512];
    sscanf(request, "GET %412s HTTP/1.1\n", req_path);

    int len = strlen(req_path);
    replace_percents(req_path, len);
    omit_query(req_path);
    puts(req_path);

    if (!strstr(req_path, "..")) {
      char path[256];
      strcpy(path, inits->root_dir);
      strcat(path, req_path);

      write_file_response(path, sockfd);
    } else {
      write_400(sockfd);
    }
  }
  close(sockfd);
}

void dispatch_POST(const char* request, int req_len, int sockfd,
  struct initvals* inits) {
  write_400(sockfd);
}

void dispatch_HEAD(const char* request, int req_len, int sockfd,
  struct initvals* inits) {
  if (req_len > 0) {
    char req_path[512];
    sscanf(request, "HEAD %412s HTTP/1.1\n", req_path);

    int len = strlen(req_path);
    replace_percents(req_path, len);
    omit_query(req_path);

    if (!strstr(req_path, "..")) {
      char path[256];
      strcpy(path, inits->root_dir);
      strcat(path, req_path);

      write_file_info(path, sockfd);
    }
  }
  close(sockfd);
}

void dispatch_other(const char* request, int req_len, int sockfd,
  struct initvals* inits) {
  write_501(sockfd);
}

typedef void (*initializer)(int, int*, char**, struct initvals*);

void nCPU_init(int argc, int* pos, char** argv, struct initvals* out) {
  out->nCPU = atoi(argv[++(*pos) ]);
}

void root_dir_init(int argc, int* pos,
  char** argv,
  struct initvals* out) {
  strncpy(out->root_dir, argv[++(*pos)], 250);
}

void port_init(int argc, int* pos, char** argv, struct initvals* out) {
  out->port = atoi(argv[++(*pos)]);
}

void initialize(int argc, char** argv,
  struct initvals* out,
  int argnum,
  const char* arguments[],
  initializer initializers[]) {
  int i = 1;
  while (i < argc) {
    for (int j = 0; j < argnum; j++) {
      puts(argv[i]);
      if (!strcmp(argv[i], arguments[j])) {
        initializers[j](argc, &i, argv, out);
        break;
      }
    }
    i++;
  }

  int plen = strlen(out->root_dir);
  if(plen > 0 && out->root_dir[plen-1] != '/') {
    out->root_dir[plen] = '/';
    out->root_dir[plen + 1] = '\0';
  }
}

void print_initial(const struct initvals* inits) {
  printf("Starging Hedgehog HTTP Server\n\
On port: %d\n\
Process number: %d\n\
Root directory: %s\n", inits->port, inits->nCPU, inits->root_dir);
}

int main(int argc, char** argv) {
  struct initvals inits = DEFAULT_INITIALS;
  const char* arguments[] = {"-r", "-c", "-p"};
  initializer initializers[] = {root_dir_init, nCPU_init, port_init};
  initialize(argc, argv, &inits, 3, arguments, initializers);

  print_initial(&inits);

  for (int i = 1; i < inits.nCPU; i++) {
    int pid = fork();
    if (pid == 0)
      break;
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(inits.port);
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
  epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);

  char buf[256];
  memset(buf, 256, 0);

  for(;;) {
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);

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
        int input_len = read(events[n].data.fd, buf, 245);
        dispatch_request(buf, input_len, events[n].data.fd, &inits);
      }
    }
  }

  return 0;
}
