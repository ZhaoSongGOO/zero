#include "mm.h"
#include <stdlib.h>

MemoryManager *new_mm() {
  MemoryManager *mm = (MemoryManager *)malloc(sizeof(MemoryManager));
  mm->allocated_memory_size = 0;
  return mm;
}