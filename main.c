#define ZERO_IMPLEMENTATION
#include "zero.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Source file not be supported, please use `zero source`");
    return 0;
  }
  compile(argv[1]);
  VM *v = new_vm();
  VM_Init(v);
  VM_Run(v);
}
