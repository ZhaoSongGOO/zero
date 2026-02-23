#define ZERO_IMPLEMENTATION
#include "zero.h"

int main() {
  struct source *s = read_source("./code.z");
  printf("%s\n", s->content);
  struct scanner *sc = scanner_init(s);
  while (sc->cur_token.type != TOKEN_EOF &&
         sc->cur_token.type != TOKEN_UNKNOWN) {
    token_print(sc->cur_token, sc);
    scanner_run(sc);
  }

  free_scanner(sc);
}
