#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "dispatch.h"
#include "filenames.h"
#include "write.h"

void dispatch_request(int sockfd, struct initvals* inits,
  struct send_data snd_arr[], int snd_len){

  struct send_data* snd_record;
  bool found = find_sockdata(sockfd, snd_arr, snd_len, &snd_record);
  if (!found) {
    dispatch_new_request(sockfd, inits, snd_record);
  }
  continue_transmit(snd_record);
}

void dispatch_new_request(int sockfd, struct initvals* inits,
  struct send_data* snd_record) {
  char request[256];
  int req_len = read(sockfd, request, 245);

  char req_type[8];
  sscanf(request, "%7s", req_type);
  if (!strcmp("GET", req_type))
    dispatch_GET(request, req_len, sockfd, inits, snd_record);
  else if (!strcmp("POST", req_type))
    dispatch_POST(request, req_len, sockfd, inits, snd_record);
  else if (!strcmp("HEAD", req_type))
    dispatch_HEAD(request, req_len, sockfd, inits, snd_record);
  else
    dispatch_other(request, req_len, sockfd, inits, snd_record);

}

void dispatch_GET(const char* request, int req_len, int sockfd,
  struct initvals* inits, struct send_data* snd_record) {
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

      write_file_response(path, sockfd, snd_record);
    } else {
      write_400(sockfd, snd_record);
    }
  }
}

void dispatch_POST(const char* request, int req_len, int sockfd,
  struct initvals* inits, struct send_data* snd_record) {
  write_400(sockfd, snd_record);
}

void dispatch_HEAD(const char* request, int req_len, int sockfd,
  struct initvals* inits, struct send_data* snd_record) {
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

      write_file_info(path, sockfd, snd_record);
    }
  } else {
    write_400(sockfd, snd_record);
  }
}

void dispatch_other(const char* request, int req_len, int sockfd,
  struct initvals* inits, struct send_data* snd_record) {
  write_501(sockfd, snd_record);
}
