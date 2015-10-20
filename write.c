#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sockdata.h"
#include "mime.h"

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

void write_header(struct send_data* snd_record,
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
  int header_size = strlen(response_header_template) + strlen(content_type) + 50;
  char* header = (char*) malloc(sizeof(char) * header_size);
  sprintf(header, response_header_template, status_code, status_string,
          content_length, content_type);

  sockdata_set_string(snd_record, header);
  free(header);
}

void write_response(int sockfd, struct send_data* snd_record,
                    int status_code, const char* status_string,
                    const char* content_type,
                    const char* buf, size_t buflen) {
  sockdata_init(snd_record, -1, sockfd);

  write_header(snd_record, status_code, status_string, buflen, content_type);
  sockdata_append(snd_record, buf);
}

void write_400(int sockfd, struct send_data* snd_record) {
  const char bad_request_message[] =
      "Bad request. Try something else.";
  write_response(sockfd, snd_record,
    400, "Bad request",
    "text/plain",
    bad_request_message, sizeof(bad_request_message) - 1);
}

void write_403(int sockfd, struct send_data* snd_record) {
  const char forbidden_message[] =
      "Access to this directory is forbidden!";
  write_response(sockfd, snd_record,
    403, "Forbidden",
    "text/plain",
    forbidden_message, sizeof(forbidden_message) - 1);
}

void write_404(int sockfd, struct send_data* snd_record) {
  const char not_found_message[] =
      "Requested file was not found on this server";
  write_response(sockfd, snd_record,
    404, "Not Found",
    "text/plain",
    not_found_message, sizeof(not_found_message) -1);
}

void write_501(int sockfd, struct send_data* snd_record) {
 const char not_implemented_message[] =
      "This method is not implemented or is unknown for server";
  write_response(sockfd, snd_record,
    501, "Not Implemented",
    "text/plain",
    not_implemented_message, sizeof(not_implemented_message) - 1);
}

void write_file_response(char* req_path, int sockfd,
  struct send_data* snd_record) {

  struct stat file_stat;
  bool is_dir = false;
  int fd = getfd_for_path(req_path, &file_stat, &is_dir);

  if (fd != -1) {
    sockdata_init(snd_record, fd, sockfd);

    char mime[64];
    int file_len = file_stat.st_size;
    get_mime_for_path(req_path, mime);

    write_header(snd_record, 200, "OK", file_len, mime);
  } else {
    if (is_dir)
      write_403(sockfd, snd_record);
    else
      write_404(sockfd, snd_record);
  }
}

void write_file_info(char* req_path, int sockfd, struct send_data* snd_record) {
  struct stat file_stat;
  bool is_dir = false;
  int fd = getfd_for_path(req_path, &file_stat, &is_dir);
  close(fd);
  puts(req_path);
  sockdata_init(snd_record, -1, sockfd);

  if (fd != -1) {
    char mime[64];
    int file_len = file_stat.st_size;
    get_mime_for_path(req_path, mime);

    const char* ok_string = "OK";
    write_header(snd_record, 200, ok_string, file_len, mime);
  } else {
    const char* not_found = "Not Found";
    write_header(snd_record, 404, not_found, 0, "");
  }
}
