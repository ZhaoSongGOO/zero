#define ZERO_IMPLEMENTATION
#include "zero.h"


int main(){
    struct token t = {.type=TOKEN_NUM, .value=12};
    struct source * s = read_source("./code.z");
    printf("%s\n", s->content);
}
