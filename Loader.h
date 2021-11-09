#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>

struct Note {
  double time;
  int8_t type;
  int8_t colorIndex;
  int8_t column;
  int8_t m_size;
};

int import(struct Note *out, char *filename, int difficulty);

#endif