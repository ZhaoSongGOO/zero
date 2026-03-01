#include "vec.h"
#include <stdlib.h>

void vec_clean_impl(struct Vec *vec) {
  for (unsigned int i = 0; i < vec->count; i++) {
    free(vec->values[i]);
  }
  free(vec->values);
  free(vec);
}

void vec_push_impl(struct Vec *vec, void *v) {
  if (vec->count < vec->capacity) {
    vec->values[vec->count] = v;
    vec->count += 1;
    return;
  }
  vec->capacity *= 2;
  vec->values = realloc(vec->values, vec->capacity * sizeof(void *));
  vec->values[vec->count] = v;
  vec->count += 1;
}

void *vec_get_impl(struct Vec *vec, unsigned int index) {
  if (index >= vec->count) {
    return NULL;
  }
  return vec->values[index];
}

struct Vec *new_vec() {
  struct Vec *vec = (struct Vec *)malloc(sizeof(struct Vec));
  vec->capacity = 10;
  vec->count = 0;
  vec->clean = vec_clean_impl;
  vec->push = vec_push_impl;
  vec->get = vec_get_impl;
  vec->values = (void **)malloc(sizeof(void *) * vec->capacity);
  return vec;
}
