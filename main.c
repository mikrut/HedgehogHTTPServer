#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>


#define DBG_INT(var) printf(#var" = %d\n", (var))
#define DBG_STR(str) puts((str))

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

int main(void) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DBG_INT(sockfd);

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  int k = bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
  DBG_INT(k);
  int l = listen(sockfd, 5);
  DBG_INT(l);

  char buf[256];
  memset(buf, 256, 0);

  for(;;) {
    DBG_STR("cycle");
    struct sockaddr_in client;
    int addrlen = sizeof(client);
    int acc = accept(sockfd, (struct sockaddr*) &client, (socklen_t*) &addrlen);
    DBG_INT(acc);

    int n = read(acc, buf, 255);
    DBG_INT(n);
    puts(buf);

    const char* resp = "Hello, world!";
    write_response(acc, 200, "OK", "text/html", resp, strlen(resp));
    close(acc);
  }

  return 0;
}
