#ifndef ZERO_MM_H
#define ZERO_MM_H
#include <unistd.h>

typedef struct zero_memory_manager {
  uint64_t allocated_memory_size;
  int dump_file_fd;
} MemoryManager;

MemoryManager *new_mm();

void memory_allocator(MemoryManager *mm, size_t size);
void memory_deallocator(MemoryManager *mm, size_t size);

#endif