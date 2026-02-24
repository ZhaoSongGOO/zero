#ifndef ZERO_VEC_H
#define ZERO_VEC_H

#include <unistd.h>
struct Vec {
  unsigned int count;
  unsigned int capacity;
  void **values;
  void (*clean)(struct Vec *vec);
  void (*push)(struct Vec *vec, void *v);
  void *(*get)(struct Vec *vec, unsigned int index);
};

struct Vec *new_vec();

#endif
