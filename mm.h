#ifndef ZERO_MM_H
#define ZERO_MM_H
#include <unistd.h>

typedef struct zero_memory_manager {
  uint64_t allocated_memory_size;
} MemoryManager;

MemoryManager *new_mm();

#endif