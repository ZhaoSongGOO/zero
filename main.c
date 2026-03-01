#define ZERO_IMPLEMENTATION
#include "zero.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Source file not be supported, please use `zero source`");
    return 0;
  }
  struct Parser *parser = parser_init(argv[1]);
  struct syntax_program *program = parser_program(parser);
  program_visitor(program);
}
