#ifndef WRITE_H
#define WRITE_H

#include "write.h"

void write_header(struct send_data* snd_record,
                 int status_code, const char* status_string,
                 int content_length, const char* content_type);
void write_response(int sockfd, struct send_data* snd_record,
                    int status_code, const char* status_string,
                    const char* content_type,
                    const char* buf, size_t buflen);

void write_400(int sockfd, struct send_data* snd_record);
void write_403(int sockfd, struct send_data* snd_record);
void write_404(int sockfd, struct send_data* snd_record);
void write_501(int sockfd, struct send_data* snd_record);

void write_file_response(char* req_path, int sockfd,
  struct send_data* snd_record);
void write_file_info(char* req_path, int sockfd, struct send_data* snd_record);


#endif
