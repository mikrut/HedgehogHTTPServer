#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "initvals.h"

const struct initvals DEFAULT_INITIALS = {
  4, "/home/mihanik/Desktop/Hedgehog HTTP Server/", 8080
};

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

const char* arguments[] = {"-r", "-c", "-p"};
const initializer initializers[] = {root_dir_init, nCPU_init, port_init};

void initialize(int argc, char** argv,
  struct initvals* out,
  int argnum,
  const char* arguments[],
  const initializer initializers[]) {
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

void init_params(int argc, char** argv, struct initvals* out_inits) {
  (*out_inits) = DEFAULT_INITIALS;
  initialize(argc, argv, out_inits, 3, arguments, initializers);
}

void print_initial(const struct initvals* inits) {
  printf("Port: %d\n\
Process number: %d\n\
Root directory: %s\n", inits->port, inits->nCPU, inits->root_dir);
}
