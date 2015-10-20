#ifndef INITVALS_H
#define INITVALS_H

struct initvals {
  int nCPU;
  char root_dir[256];
  int port;
};

extern const struct initvals DEFAULT_INITIALS;

void init_params(int argc, char** argv, struct initvals* out_inits);
void print_initial(const struct initvals* inits);

#endif
