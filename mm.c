#include "mm.h"
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>    
#include <unistd.h>   
#include <string.h>   
#include <stdio.h>
#include <time.h>

void get_dump_file_name(char *buffer, size_t max_len) {
  time_t raw_time;
  struct tm *time_info;
  time(&raw_time);
  time_info = localtime(&raw_time);
  strftime(buffer, max_len, "%Y_%m_%d_%H_%M_%S.log", time_info);
}

#define KB 1024
#define MB (1024 * 1024)

#define DUMP_THRESHOLD (10 * MB)

MemoryManager *new_mm() {
  MemoryManager *mm = (MemoryManager *)malloc(sizeof(MemoryManager));
  mm->allocated_memory_size = 0;
  mm->dump_file_fd = -1;
  return mm;
}

void DUMP_MEMORY(MemoryManager *mm, const char *fmt, ...) {
  if (fmt == NULL) {
    return;
  }
  char *buf = NULL;

  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  if (len < 0)
    return;
  buf = malloc(len + 1);
  va_start(args, fmt);
  vsnprintf(buf, len + 1, fmt, args);
  va_end(args);

  if (mm->dump_file_fd == -1) {
    char * dump_file_name = (char*)malloc(sizeof(char) * 30);
    get_dump_file_name(dump_file_name, 30);
    mm->dump_file_fd = open(dump_file_name, O_WRONLY | O_APPEND | O_CREAT, 0644);
  }

  if (mm->dump_file_fd == -1) {
    return;
  }
  write(mm->dump_file_fd, buf, strlen(buf));
}

void memory_allocator(MemoryManager *mm, size_t size) {
  mm->allocated_memory_size += size;
  if(mm->allocated_memory_size >= DUMP_THRESHOLD){
    DUMP_MEMORY(mm, "Memory %d Byte\n", mm->allocated_memory_size);
  }
}
void memory_deallocator(MemoryManager *mm, size_t size) {
  if (mm->allocated_memory_size < size) {
    mm->allocated_memory_size = 0;
  } else {
    mm->allocated_memory_size -= size;
  }
  if(mm->allocated_memory_size >= DUMP_MEMORY){
    DUMP_MEMORY(mm, "Memory %d Byte\n", mm->allocated_memory_size);
  }
}