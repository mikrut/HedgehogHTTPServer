#ifndef SOCKDATA_H
#define SOCKDATA_H

#include <stdbool.h>

struct send_data {
  int sockfd;
  int fd;
  char* buf;
  int bsize;
  int bpos;
  int blen;
  bool waited;
};

void sockdata_initarr(struct send_data sockdata[], int arr_len);

void sockdata_init(struct send_data* sockdata, int fd, int sockfd);

bool find_sockdata(int sockfd, struct send_data sockdata[], int arrlen,
                  struct send_data** out);

void sockdata_set_string(struct send_data* sockdata, const char* str);
void sockdata_append(struct send_data* sockdata, const char* str);

void continue_transmit(struct send_data* sockdata, int *snd_waitors);
void read_data(struct send_data* sockdata);

#endif
