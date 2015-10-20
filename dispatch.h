#ifndef DISPATCH_H
#define DISPATCH_H

#include "sockdata.h"
#include "initvals.h"

void dispatch_request(int sockfd, struct initvals* inits,
  struct send_data snd_arr[], int snd_len, int* snd_waitors);


void dispatch_new_request(int sockfd, struct initvals* inits,
  struct send_data* snd_record);

void dispatch_GET(const char* request, int req_len, int sockfd,
  struct initvals* inits, struct send_data* snd_record);
void dispatch_POST(const char* request, int req_len, int sockfd,
  struct initvals* inits, struct send_data* snd_record);
void dispatch_HEAD(const char* request, int req_len, int sockfd,
  struct initvals* inits, struct send_data* snd_record);
void dispatch_other(const char* request, int req_len, int sockfd,
  struct initvals* inits, struct send_data* snd_record);

#endif
