#define ZERO_IMPLEMENTATION
#include "zero.h"

char *target_filename(const char *source) {
  const char *dot = strrchr(source, '.');
  int base_len;

  if (dot != NULL) {
    base_len = dot - source;
  } else {
    base_len = strlen(source);
  }

  char *target = malloc(base_len + 4);
  if (!target)
    return NULL;
  snprintf(target, base_len + 4, "%.*s.ir", base_len, source);
  return target;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Source file not be supported, please use `zero source`");
    return 0;
  }
  struct Parser *parser = parser_init(argv[1]);
  struct syntax_program *program = parser_program(parser);
  GET_INSTRUCTION_STORE()->fd =
      open(target_filename(argv[1]), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  program_visitor(program);
  instruction_to_file();
  VM *v = new_vm();
  VM_Init(v);
  VM_Run(v);
}
