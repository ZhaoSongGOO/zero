#ifndef __ZERO_PUBLIC_H__
#define __ZERO_PUBLIC_H__

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "map.h"
#include "mm.h"
#include "vec.h"

typedef enum {
  TOKEN_UNKNOWN = -1,
  TOKEN_STRING,
  TOKEN_NUM,
  TOKEN_ADD,
  TOKEN_MINUS,
  TOKEN_DIV,
  TOKEN_MULTI,
  TOKEN_ASSIGN,
  TOKEN_ID,
  TOKEN_KEYWORD,
  TOKEN_FUNC,
  TOKEN_RETURN,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_INCLUDE,
  TOKEN_NULL,
  TOKEN_EQUAL,         // ==
  TOKEN_NOT_EQUAL,     // !=
  TOKEN_AND,           // &&
  TOKEN_GREATER,       // >
  TOKEN_LESS,          // <
  TOKEN_GREATER_EQUAL, // >=
  TOKEN_LESS_EQUAL,    // <=
  TOKEN_OR,            // ||
  TOKEN_NOT,           // !
  TOKEN_ACCESS,        // .
  TOKEN_COMMA,         // ,
  TOKEN_SEMICOLON,     // ;
  TOKEN_LEFT_PARENT,   // (
  TOKEN_RIGHT_PARENT,  // )
  TOKEN_LEFT_BRACKET,  // [
  TOKEN_RIGHT_BRACKET, // ]
  TOKEN_LEFT_BRACE,    // {
  TOKEN_RIGHT_BRACE,   // }
  TOKEN_DOUBLE_QUOTES, // "
  TOKEN_COLON,         // :
  TOKEN_TYPE_DEF,      // typedef
  TOKEN_NEW,           // new
  TOKEN_THIS,          // this
  TOKEN_EOF
} TOKEN_TYPE;

void ZERO_ASSERT(bool condition, const char *fmt, ...) {
  if (condition) {
    return;
  }

  char *buf = NULL;
  if (fmt != NULL) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len < 0) {
      assert(false);
    }
    buf = malloc(len + 1);
    va_start(args, fmt);
    vsnprintf(buf, len + 1, fmt, args);
    va_end(args);
  }

  printf("[FATAL]: %s\n", buf);
  assert(false);
}

typedef enum { NUMBER_INT, NUMBER_FLOAT, NUMBER_BOOL } NUMBER_Type;

struct number {
  NUMBER_Type type;
  union {
    int int_value;
    double float_value;
    bool bool_value;
  };
};

struct token_info {
  int col;
  int row;
};

struct token {
  TOKEN_TYPE type;
  union {
    int symbol_index;
    struct number number_value;
  } value;
  struct token_info info;
};

struct str_item {
  char *str;
  struct str_item *next;
  struct str_item *prev;
};

struct str_store {
  unsigned int count;
  struct str_item *head;
  struct str_item *tail;
  struct str_item *(*get)(struct str_store *self, unsigned int index);
  unsigned int (*insert)(struct str_store *self, const char *src,
                         unsigned int size);
  unsigned int (*insert_raw)(struct str_store *self, const char *src);
  void (*remove_end)(struct str_store *self);
};

struct source {
  const char *content;
  unsigned int size;
};

struct scanner {
  struct source *source;
  struct token cur_token;
  struct token next_token;
  struct str_store *symbol;
  unsigned int index;
  int col;
  int row;
};

struct scanner *scanner_init(struct source *source_file);

struct token next_token(struct scanner *s);

void scanner_run(struct scanner *s);

struct source *read_source(const char *file_path);
void free_source(struct source *source_file);
void free_scanner(struct scanner *scanner);

struct str_item *str_store_get_impl(struct str_store *self, unsigned int index);

unsigned int str_store_insert_raw_impl(struct str_store *self, const char *src);

unsigned int str_store_insert_impl(struct str_store *self, const char *src,
                                   unsigned int size);

void remove_end_impl(struct str_store *self);

struct str_store *str_store_init();

void free_str_store(struct str_store *);

struct str_item *free_str_item(struct str_item *item);

void token_print(struct token, struct scanner *sc);

void zero_cmd(int argc, char *argv[]);

void compile(const char *file_name);

void run();

#ifdef ZERO_IMPLEMENTATION

struct source *read_source(const char *file_path) {
  if (file_path == NULL || strlen(file_path) == 0) {
    ZERO_ASSERT(!(file_path == NULL || strlen(file_path) == 0),
                "source file name is error!");
  }

  int fd = open(file_path, O_RDONLY | O_CLOEXEC);
  ZERO_ASSERT(!(fd == -1), "source file (%s) open failed!", file_path);

  off_t file_size = lseek(fd, 0, SEEK_END);
  if (file_size == -1) {
    close(fd);
    ZERO_ASSERT(false, "source file (%s) size if -1!", file_path);
  }
  lseek(fd, 0, SEEK_SET);

  char *file_content = (char *)malloc(file_size + 1);
  if (file_content == NULL) {
    close(fd);
    ZERO_ASSERT(false, "malloc failed for read source %s!", file_path);
  }

  ssize_t read_bytes = read(fd, file_content, file_size);
  if (read_bytes == -1) {
    free(file_content);
    close(fd);
    ZERO_ASSERT(false, "source file (%s) read failed!", file_path);
  }
  file_content[read_bytes] = '\0';
  close(fd);

  struct source *src = (struct source *)malloc(sizeof(struct source));
  src->content = file_content;
  src->size = read_bytes;
  return src;
}

void zero_logo() {
  printf("* * * * * * * * * * * * * * * * * * * **\n");
  printf("*   ______  ______   _____     ____    *\n");
  printf("*  |___  / | _____| |  __ \\   / __ \\   *\n");
  printf("*     / /  | |__    | |__) | | |  | |  *\n");
  printf("*    / /   |  __|   |  _  /  | |  | |  *\n");
  printf("*   / /__  | |____  | | \\ \\  | |__| |  *\n");
  printf("*  /_____| |______| |_|  \\_\\  \\____/   *\n");
  printf("*                                   LMM*\n");
  printf("* * * * * * * * * * * * * * * * * * * **\n\n\n");
}

void zero_help() {
  zero_logo();
  printf("Usage: zero [options] <input_file>\n\n");
  printf("Options:\n");
  printf("  -h, --help              Show this help message\n");
  printf("  -v, --version           Display version information\n");
  printf("  -i, --input <file>      Specify the input files\n");
  printf("\nExample:\n");
  printf("  zero -i main.z\n");
}

void zero_cmd(int argc, char *argv[]) {
  static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                         {"version", no_argument, 0, 'v'},
                                         {"input", required_argument, 0, 'i'},
                                         {0, 0, 0, 0}};
  int opt;
  char *input_file = NULL;
  while ((opt = getopt_long(argc, argv, "hvi:", long_options, NULL)) != -1) {
    switch (opt) {
    case 'h':
      zero_help();
      return;
    case 'v':
      printf("zero compiler version 1.0.0-stable\n");
      return;
    case 'i':
      input_file = optarg;
      break;
    case '?':
      // getopt_long already prints an error message
      return;
    default:
      abort();
    }
  }

  if (input_file != NULL) {
    compile(input_file);
    run();
  } else {
    zero_help();
    fprintf(stderr, "Error: No input files provided.\n");
    return;
  }
}

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

void free_source(struct source *source_file) {
  if (source_file->content != NULL) {
    free(source_file->content);
  }
  source_file->size = 0;
  free(source_file);
}

struct scanner *scanner_init(struct source *source_file) {
  struct scanner *s = (struct scanner *)malloc(sizeof(struct scanner));
  s->source = source_file;
  s->index = 0;
  s->symbol = str_store_init();
  s->cur_token = next_token(s);
  s->next_token = next_token(s);
  s->col = 0;
  s->row = 0;
  return s;
}

bool is_white_space(char c) { return c == ' ' || c == '\n' || c == '\t'; }

bool is_number(char c) { return c >= '0' && c <= '9'; }

bool is_letter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

bool is_opcode(char c) {
  return c == '"' || c == ';' || c == ':' || c == '+' || c == '-' || c == '*' ||
         c == '/' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' ||
         c == '}' || c == '=' || c == ',' || c == '.' || c == '!' || c == '>' ||
         c == '<' || c == '&' || c == '|';
}

bool is_type(const char *str) {}

bool is_keyword(const char *str) {
  return strcmp(str, "var") == 0 || strcmp(str, "true") == 0 ||
         strcmp(str, "false") == 0 || strcmp(str, "func") == 0 ||
         strcmp(str, "return") == 0 || strcmp(str, "if") == 0 ||
         strcmp(str, "else") == 0 || strcmp(str, "while") == 0 ||
         strcmp(str, "include") == 0 || strcmp(str, "null") == 0 ||
         strcmp(str, "typedef") == 0 || strcmp(str, "new") == 0 ||
         strcmp(str, "this") == 0;
}

struct token scanner_letter(struct scanner *s) {
  unsigned int size = 0;
  unsigned int index = s->index;
  if (is_letter(s->source->content[index])) {
    size += 1;
    index += 1;
  }
  while (is_letter(s->source->content[index]) ||
         is_number(s->source->content[index])) {
    size += 1;
    index += 1;
  }
  unsigned int si =
      s->symbol->insert(s->symbol, s->source->content + s->index, size);
  s->index = s->index + size;
  struct token_info info = {.col = s->col, .row = s->row};
  s->col += size;
  const char *str = s->symbol->get(s->symbol, si)->str;
  if (is_keyword(str)) {
    if (strcmp(str, "true") == 0) {
      return (struct token){
          .type = TOKEN_NUM,
          .value = {.number_value = {.type = NUMBER_BOOL, .bool_value = true}},
          .info = info};
    }
    if (strcmp(str, "false") == 0) {
      return (struct token){
          .type = TOKEN_NUM,
          .value = {.number_value = {.type = NUMBER_BOOL, .bool_value = false}},
          .info = info};
    }
    if (strcmp(str, "func") == 0) {
      return (struct token){.type = TOKEN_FUNC, .info = info};
    }
    if (strcmp(str, "return") == 0) {
      return (struct token){.type = TOKEN_RETURN, .info = info};
    }
    if (strcmp(str, "if") == 0) {
      return (struct token){.type = TOKEN_IF, .info = info};
    }
    if (strcmp(str, "else") == 0) {
      return (struct token){.type = TOKEN_ELSE, .info = info};
    }
    if (strcmp(str, "while") == 0) {
      return (struct token){.type = TOKEN_WHILE, .info = info};
    }

    if (strcmp(str, "include") == 0) {
      return (struct token){.type = TOKEN_INCLUDE, .info = info};
    }

    if (strcmp(str, "null") == 0) {
      return (struct token){.type = TOKEN_NULL, .info = info};
    }

    if (strcmp(str, "typedef") == 0) {
      return (struct token){.type = TOKEN_TYPE_DEF, .info = info};
    }

    if (strcmp(str, "new") == 0) {
      return (struct token){.type = TOKEN_NEW, .info = info};
    }

    if (strcmp(str, "this") == 0) {
      return (struct token){.type = TOKEN_THIS, .info = info};
    }

    return (struct token){
        .type = TOKEN_KEYWORD, .value = {.symbol_index = si}, .info = info};
  }
  return (struct token){
      .type = TOKEN_ID, .value = {.symbol_index = si}, .info = info};
}

struct token scanner_number(struct scanner *s) {
  unsigned int size = 0;
  unsigned int index = s->index;
  bool is_float = false;
  while (is_number(s->source->content[index]) ||
         s->source->content[index] == '.') {
    if (s->source->content[index] == '.') {
      if (is_float) {
        return (struct token){.type = TOKEN_UNKNOWN};
      } else {
        is_float = true;
      }
    }
    index++;
    size++;
  }
  struct token_info info = {.col = s->col, .row = s->row};
  if (!is_float) {
    unsigned int value = 0;
    unsigned int digital = 1;
    unsigned int patch = 1;
    while (patch <= size) {
      value += (s->source->content[index - patch] - '0') * digital;
      digital *= 10;
      patch += 1;
    }
    s->index += size;
    s->col += size;
    return (struct token){
        .type = TOKEN_NUM,
        .value = {.number_value =
                      (struct number){.type = NUMBER_INT, .int_value = value}},
        .info = info};
  } else {
    double float_value = strtod(s->source->content + s->index, NULL);
    s->index += size;
    s->col += size;
    return (struct token){
        .type = TOKEN_NUM,
        .value = {.number_value = (struct number){.type = NUMBER_FLOAT,
                                                  .float_value = float_value}},
        .info = info};
  }
}

struct token scanner_string(struct scanner *s) {
  s->index++; // skip '"'
  unsigned size = 0;
  unsigned index = s->index;
  while (index < s->source->size && s->source->content[index] != '"') {
    index++;
    size++;
  }
  if (index >= s->source->size) {
    return (struct token){.type = TOKEN_UNKNOWN};
  }

  unsigned int si =
      s->symbol->insert(s->symbol, s->source->content + s->index, size);
  struct token_info info = {.col = s->col, .row = s->row};
  s->index = s->index + size + 1; // +1 to skip '"'
  s->col += size + 1;
  return (struct token){
      .type = TOKEN_STRING, .value = {.symbol_index = si}, .info = info};
}

void single_comment_consumer(struct scanner *s) {
  int first = s->index + 2;
  while (first < s->source->size) {
    if (s->source->content[first] == '\n') {
      s->index = first + 1;
      s->col = 0;
      s->row += 1;
      return;
    }
    first += 1;
  }
}

void multi_comment_consumer(struct scanner *s) {
  int first = s->index + 2;
  int second = first + 1;
  while (first < s->source->size && second < s->source->size) {
    if (s->source->content[first] == '*' && s->source->content[second] == '/') {
      s->index = second + 1;
      return;
    }
    if (s->source->content[first] == '\n') {
      s->col = 0;
      s->row += 1;
    }
    s->col += 1;
    first += 1;
    second = first + 1;
  }
  assert(false);
}

void comment_consumer(struct scanner *s) {
  int first = s->index;
  int second = s->index + 1;
  if (first >= s->source->size || second >= s->source->size) {
    assert(false);
  }
  if (s->source->content[second] == '/') {
    single_comment_consumer(s);
  } else if (s->source->content[second] == '*') {
    multi_comment_consumer(s);
  }
}

struct token scanner_opcode(struct scanner *s) {
  struct token t = {.type = TOKEN_UNKNOWN};
  char next = s->source->content[s->index + 1];
  int start = s->index;
  switch (s->source->content[s->index]) {
  case '+':
    t = (struct token){.type = TOKEN_ADD};
    break;
  case '-':
    t = (struct token){.type = TOKEN_MINUS};
    break;
  case '*':
    t = (struct token){.type = TOKEN_MULTI};
    break;
  case '/': {
    if (next == '/' || next == '*') {
      comment_consumer(s);
      return next_token(s);
    } else {
      t = (struct token){.type = TOKEN_DIV};
    }
  } break;
  case '(':
    t = (struct token){.type = TOKEN_LEFT_PARENT};
    break;
  case ')':
    t = (struct token){.type = TOKEN_RIGHT_PARENT};
    break;
  case '[':
    t = (struct token){.type = TOKEN_LEFT_BRACKET};
    break;
  case ']':
    t = (struct token){.type = TOKEN_RIGHT_BRACKET};
    break;
  case '{':
    t = (struct token){.type = TOKEN_LEFT_BRACE};
    break;
  case '}':
    t = (struct token){.type = TOKEN_RIGHT_BRACE};
    break;
  case '=': {
    if (next == '=') {
      t = (struct token){.type = TOKEN_EQUAL};
      s->index += 1;
    } else {
      t = (struct token){.type = TOKEN_ASSIGN};
    }
  } break;
  case '&': {
    if (next == '&') {
      t = (struct token){.type = TOKEN_AND};
      s->index += 1;
    } else {
      assert(false);
    }
  } break;
  case '|': {
    if (next == '|') {
      t = (struct token){.type = TOKEN_OR};
      s->index += 1;
    } else {
      assert(false);
    }
  } break;
  case '!': {
    if (next == '=') {
      t = (struct token){.type = TOKEN_NOT_EQUAL};
      s->index += 1;
    } else {
      t = (struct token){.type = TOKEN_NOT};
    }
  } break;
  case '>': {
    if (next == '=') {
      t = (struct token){.type = TOKEN_GREATER_EQUAL};
      s->index += 1;
    } else {
      t = (struct token){.type = TOKEN_GREATER};
    }
  } break;
  case '<': {
    if (next == '=') {
      t = (struct token){.type = TOKEN_LESS_EQUAL};
      s->index += 1;
    } else {
      t = (struct token){.type = TOKEN_LESS};
    }
  } break;
  case ':':
    t = (struct token){.type = TOKEN_COLON};
    break;
  case ';':
    t = (struct token){.type = TOKEN_SEMICOLON};
    break;
  case ',':
    t = (struct token){.type = TOKEN_COMMA};
    break;
  case '.':
    t = (struct token){.type = TOKEN_ACCESS};
    break;
  case '"': // for string
    return scanner_string(s);

  default:
    return t;
  }
  s->index += 1;
  t.info = (struct token_info){.col = s->col, .row = s->row};
  s->col += s->index - start;
  return t;
}

struct token next_token(struct scanner *s) {
  if (s->index >= s->source->size) {
    return (struct token){.type = TOKEN_EOF,
                          .info = {.col = s->col, .row = s->row}};
  }
  // skip white space
  while (is_white_space(s->source->content[s->index])) {
    if (s->source->content[s->index] == '\n') {
      s->col = 0;
      s->row += 1;
    }
    s->index += 1;
  }

  char c = s->source->content[s->index];
  if (is_letter(c)) {
    return scanner_letter(s);
  } else if (is_number(c)) {
    return scanner_number(s);
  } else if (is_opcode(c)) {
    return scanner_opcode(s);
  } else if (c == '\0') {
    return (struct token){.type = TOKEN_EOF,
                          .info = {.col = s->col, .row = s->row}};
  } else {
    return (struct token){.type = TOKEN_UNKNOWN,
                          .info = {.col = s->col, .row = s->row}};
  }
}

void scanner_run(struct scanner *s) {
  if (s->cur_token.type == TOKEN_EOF) {
    return;
  }
  s->cur_token = s->next_token;
  s->next_token = next_token(s);
}

void free_scanner(struct scanner *s) {
  free_source(s->source);
  free_str_store(s->symbol);
}

struct str_item *str_store_get_impl(struct str_store *self,
                                    unsigned int index) {
  if (index >= self->count) {
    return NULL;
  }
  unsigned int start = 0;
  struct str_item *p = self->head;
  while (start < index) {
    p = p->next;
    start += 1;
  }
  return p;
}

unsigned int str_store_insert_raw_impl(struct str_store *self,
                                       const char *src) {
  struct str_item *p = (struct str_item *)malloc(sizeof(struct str_item));
  p->next = NULL;
  p->prev = NULL;
  p->str = src;
  if (self->tail == NULL) {
    self->head = p;
    self->tail = p;
  } else {
    self->tail->next = p;
    p->prev = self->tail;
    self->tail = p;
  }
  self->count += 1;
  return self->count - 1;
}

void remove_end_impl(struct str_store *self) {
  if (self->count == 0) {
    return;
  }
  if (self->count == 1) {
    free(self->head->str);
    free(self->head);
    self->count = 0;
    self->head = NULL;
    self->tail = NULL;
  }

  self->tail = self->tail->prev;
  free(self->tail->next->str);
  free(self->tail->next);
  self->count -= 1;
}

unsigned int str_store_insert_impl(struct str_store *self, const char *src,
                                   unsigned int size) {
  struct str_item *p = (struct str_item *)malloc(sizeof(struct str_item));
  p->next = NULL;
  p->prev = NULL;
  p->str = (char *)malloc(sizeof(char) * (size + 1));
  strncpy(p->str, src, size);
  *(p->str + size) = '\0';
  if (self->tail == NULL) {
    self->head = p;
    self->tail = p;
  } else {
    self->tail->next = p;
    p->prev = self->tail;
    self->tail = p;
  }
  self->count += 1;
  return self->count - 1;
}

struct str_store *str_store_init() {
  struct str_store *store =
      (struct str_store *)malloc(sizeof(struct str_store));
  store->count = 0;
  store->head = NULL;
  store->tail = NULL;
  store->get = str_store_get_impl;
  store->insert = str_store_insert_impl;
  store->insert_raw = str_store_insert_raw_impl;
  store->remove_end = remove_end_impl;
  return store;
}

void free_str_store(struct str_store *store) {
  if (store == NULL) {
    return;
  }
  struct str_item *p = store->head;
  while (p != NULL) {
    p = free_str_item(p);
  }

  free(store);
}

struct str_item *free_str_item(struct str_item *item) {
  free(item->str);
  struct str_item *n = item->next;
  free(item);
  return n;
}

void token_print(struct token t, struct scanner *sc) {
  switch (t.type) {
  case TOKEN_KEYWORD:
    printf("TOKEN_KEYWORD:%s\n",
           sc->symbol->get(sc->symbol, t.value.symbol_index)->str);
    break;
  case TOKEN_ID:
    printf("TOKEN_ID:%s\n",
           sc->symbol->get(sc->symbol, t.value.symbol_index)->str);
    break;
  case TOKEN_STRING:
    printf("TOKEN_STRING:\"%s\"\n",
           sc->symbol->get(sc->symbol, t.value.symbol_index)->str);
    break;
  case TOKEN_NUM: {
    if (t.value.number_value.type == NUMBER_INT) {
      printf("TOKEN_NUM:i(%d)\n", t.value.number_value.int_value);
    } else if (t.value.number_value.type == NUMBER_FLOAT) {
      printf("TOKEN_NUM:f(%f)\n", t.value.number_value.float_value);
    }
  } break;
  case TOKEN_ADD:
    printf("TOKEN_ADD:+\n");
    break;
  case TOKEN_MINUS:
    printf("TOKEN_MINUS:-\n");
    break;
  case TOKEN_MULTI:
    printf("TOKEN_MULTI:*\n");
    break;
  case TOKEN_DIV:
    printf("TOKEN_DIV:/\n");
    break;
  case TOKEN_ASSIGN:
    printf("TOKEN_ASSIGN:=\n");
    break;
  case TOKEN_COLON:
    printf("TOKEN_COLON: : \n");
    break;
  case TOKEN_SEMICOLON:
    printf("TOKEN_SEMICOLON:; \n");
    break;
  case TOKEN_LEFT_PARENT:
    printf("TOKEN_LEFT_PARENT:( \n");
    break;
  case TOKEN_RIGHT_PARENT:
    printf("TOKEN_LEFT_PARENT:) \n");
    break;
  case TOKEN_LEFT_BRACKET:
    printf("TOKEN_LEFT_PARENT:[ \n");
    break;
  case TOKEN_RIGHT_BRACKET:
    printf("TOKEN_LEFT_PARENT:] \n");
    break;
  case TOKEN_LEFT_BRACE:
    printf("TOKEN_LEFT_BRACE: { \n");
    break;
  case TOKEN_RIGHT_BRACE:
    printf("TOKEN_RIGHT_BRACE:} \n");
    break;
  case TOKEN_COMMA:
    printf("TOKEN_COMMA:, \n");
    break;
  case TOKEN_ACCESS:
    printf("TOKEN_ACCESS:. \n");
    break;
  default:
    printf("TOKEN_UNKNOWN:%d\n", t.type);
    break;
  }
}

// Syntax chapter

void expected_token_type_and_run(struct scanner *sc, TOKEN_TYPE type) {
  // printf("DEBUG: %d--%d\n", sc->cur_token.type, type);
  assert(sc->cur_token.type == type);
  scanner_run(sc);
}

void soft_expected_token_type_and_run(struct scanner *sc, TOKEN_TYPE type) {
  // printf("DEBUG: soft %d--%d\n", sc->cur_token.type, type);
  if (sc->cur_token.type == type) {
    scanner_run(sc);
  }
}

void expected_token_type_str_value_and_run(struct scanner *sc, TOKEN_TYPE type,
                                           const char *value) {
  assert(sc->cur_token.type == type);
  assert(strcmp(value,
                sc->symbol->get(sc->symbol, sc->cur_token.value.symbol_index)
                    ->str) == 0);
  scanner_run(sc);
}

struct syntax_expr;

struct syntax_kv_pair {
  char *key;
  struct syntax_expr *value;
};

struct syntax_expr {
  enum {
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_LITERAL,
    EXPR_CALL,
    EXPR_ARRAY,
    EXPR_OBJECT,
    EXPR_ID,
    EXPR_ACCESS,
    EXPR_NEW
  } type;

  union {
    struct {
      struct syntax_expr *left;
      uint8_t op;
      struct syntax_expr *right;
    } binary_expr;

    struct {
      int var_index;
      bool is_from_params;
      int offset;

      bool source_in_stack;
      char *func_name;
      // struct syntax_expr **args;
      // uint8_t arg_count;
      struct Vec *args;
    } call_expr;

    struct {
      // struct syntax_expr **elements;
      // uint8_t count;
      struct Vec *elements;
    } array_expr;

    struct {
      // struct syntax_kv_pair **pairs;
      // uint8_t count;
      struct Vec *pairs;
    } object_expr;

    struct {
      bool from_params;
      int offset;
      char *name;
      int var_index;
      bool is_from_top;     // if var defined from global.
      bool may_be_function; // {"a":f}, `f` may be a function.
      bool is_this;
      bool is_in_access;
    } identifier_expr;

    struct {
      struct syntax_expr *obj;
      struct syntax_expr *prop;
    } access_expr;

    struct {
      struct type_def_meta_data *mete_data;
      Map *init_list;
    } new_expr;

    struct {
      enum { LIT_INT, LIT_FLOAT, LIT_STR, LIT_BOOL, LIT_NULL } kind;

      union {
        int32_t int_val;
        double float_val;
        char *str_val;
        bool bool_val;
      };
    } literal_expr;
  } data;
};

struct syntax_statement {
  enum {
    STMT_VAR_DECL,
    STMT_EXPR,
    STMT_RETURN,
    STMT_IF,
    STMT_BLOCK,
    STMT_WHILE,
    STMT_INCLUDE
  } type;

  union {
    struct {
      char *name;
      int var_index;
      struct syntax_expr *initializer;
    } var_stmt;

    struct {
      struct syntax_expr *expr;
      bool need_pop;
    } expr_stmt;

    struct {
      struct syntax_expr *expr;
    } ret_stmt;

    struct {
      struct syntax_expr *expression;
      struct syntax_statement *if_block;
      struct syntax_statement *else_block;
    } if_stmt;

    struct {
      struct Vec *statements;
      int var_count;
    } block_stmt;

    struct {
      struct syntax_expr *expression;
      struct syntax_statement *while_block;
    } while_stmt;

    struct {
      const char *include_path;
    } include_stmt;
  } data;
};

struct syntax_function_define {
  const char *name;
  Map *params; // name -> index
  struct Vec *statements;
};

struct syntax_function_define *new_function_define(const char *name) {
  struct syntax_function_define *fd = (struct syntax_function_define *)malloc(
      sizeof(struct syntax_function_define));
  fd->params = new_map();
  fd->statements = new_vec();
  fd->name = name;
  return fd;
}

struct syntax_top_module {
  enum {
    STATEMENT,
    FUNC_DEFINE,
  } type;
  union {
    struct syntax_statement *statement;
    struct syntax_function_define *func_define;
  } data;
};

struct syntax_program {
  // struct syntax_statement **statements;
  // unsigned int count;
  // unsigned int capacity;
  struct Vec *modules;
  struct Vec *include_paths;
};

struct zero_type {
  enum {
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STR,
    TYPE_OBJ,
    TYPE_CUSTOM
  } type;

  const char *name;
};

struct zero_type *get_zero_type(const char *str) {
  struct zero_type *t = (struct zero_type *)malloc(sizeof(struct zero_type));
  if (strcmp(str, "int") == 0) {
    t->type = TYPE_INT;
  } else if (strcmp(str, "float") == 0) {
    t->type = TYPE_FLOAT;
  } else if (strcmp(str, "bool") == 0) {
    t->type = TYPE_BOOL;
  } else if (strcmp(str, "string") == 0) {
    t->type = TYPE_STR;
  } else if (strcmp(str, "object") == 0) {
    t->type = TYPE_OBJ;
  } else {
    t->type = TYPE_CUSTOM;
    t->name = str;
  }
  return t;
}

struct type_def_meta_data {
  const char *type_name;
  Map *segments;
};

struct type_def_meta_data *new_type_def_meta_data() {
  struct type_def_meta_data *r =
      (struct type_def_meta_data *)malloc(sizeof(struct type_def_meta_data));
  r->segments = new_map();
  return r;
}

struct parser_scope {
  struct parser_scope *parent;
  Map *variables;
  Map *params;
  Map *type_defs;
  bool is_top_scope;
  bool is_root_scope;
  int top_var_count;
};

struct Parser {
  struct scanner *sc;
  struct parser_scope *cur_scope;
  struct parser_scope *root_scope;
};

static struct Parser *GLOBAL_PARSER = NULL;

struct parser_scope *new_parser_scope() {
  struct parser_scope *psp =
      (struct parser_scope *)malloc(sizeof(struct parser_scope));
  psp->parent = NULL;
  psp->variables = new_map();
  psp->params = new_map();
  psp->is_top_scope = false;
  psp->is_root_scope = false;
  psp->top_var_count = 0;
  psp->type_defs = new_map();
  return psp;
}

struct Parser *parser_init() {
  struct Parser *p = (struct Parser *)malloc(sizeof(struct Parser));
  p->sc = NULL;
  p->root_scope = new_parser_scope();
  p->root_scope->is_root_scope = true;
  p->cur_scope = p->root_scope;
  return p;
}

struct Parser *parser_reload(struct Parser *parser, const char *source_name) {
  struct source *s = read_source(source_name);

  if (s == NULL) {
    printf("file %s not found", source_name);
  }
  parser->cur_scope = parser->root_scope;
  parser->sc = scanner_init(s);
  struct parser_scope *psp = new_parser_scope();
  psp->is_top_scope = true;
  psp->parent = parser->cur_scope;
  parser->cur_scope = psp;
  return parser;
}

struct syntax_statement *parser_var_decl_stmt(struct Parser *parser);
struct syntax_statement *parser_expr_stmt(struct Parser *parser);
struct syntax_statement *parser_ret_stmt(struct Parser *parser);
struct syntax_statement *parser_block_statement(struct Parser *parser);
struct syntax_statement *parser_if_statement(struct Parser *parser);
struct syntax_statement *parser_while_statement(struct Parser *parser);
struct syntax_statement *parser_include_statement(struct Parser *parser);
struct syntax_statement *parser_statement(struct Parser *parser);
struct syntax_function_define *parser_function_define(struct Parser *parser);
struct syntax_expr *parser_assignment_expr(struct Parser *parser);
struct syntax_expr *parser_logic_or(struct Parser *parser);
struct syntax_expr *parser_logic_and(struct Parser *parser);
struct syntax_expr *parser_equality(struct Parser *parser);
struct syntax_expr *parser_comparison(struct Parser *parser);
struct syntax_expr *parser_assignment(struct Parser *parser);
struct syntax_expr *parser_term(struct Parser *parser);
struct syntax_expr *parser_factory(struct Parser *parser);
struct syntax_expr *parser_access(struct Parser *parser);
struct syntax_expr *parser_call(struct syntax_expr *caller,
                                struct Parser *parser);
struct syntax_expr *parser_primary_call(struct Parser *parser);
struct syntax_expr *parser_primary(struct Parser *parser);
struct syntax_expr *parser_object_access(struct Parser *parser);
struct syntax_expr *parser_arraylist(struct Parser *parser);
struct syntax_expr *parser_objectlist(struct Parser *parser);
struct syntax_expr *parser_expr(struct Parser *parser);
struct syntax_expr *parser_new(struct Parser *parser);
void parser_type_def(struct Parser *parser);

struct syntax_program *parser_program(struct Parser *parser) {
  struct syntax_program *program =
      (struct syntax_program *)malloc(sizeof(struct syntax_program));
  program->include_paths = new_vec();
  program->modules = new_vec();
  while (parser->sc->cur_token.type != TOKEN_EOF) {
    if (parser->sc->cur_token.type == TOKEN_FUNC) {
      struct parser_scope *psp = new_parser_scope();
      psp->parent = parser->cur_scope;
      parser->cur_scope = psp;
      struct syntax_function_define *func = parser_function_define(parser);
      parser->cur_scope = parser->cur_scope->parent;
      struct syntax_top_module *module =
          (struct syntax_top_module *)malloc(sizeof(struct syntax_top_module));
      module->type = FUNC_DEFINE;
      module->data.statement = func;
      program->modules->push(program->modules, module);
    } else if (parser->sc->cur_token.type == TOKEN_TYPE_DEF) {
      parser_type_def(parser);
    } else {
      struct syntax_statement *stmt = parser_statement(parser);
      if (stmt->type == STMT_INCLUDE) {
        program->include_paths->push(program->include_paths,
                                     stmt->data.include_stmt.include_path);
        continue;
      }
      struct syntax_top_module *module =
          (struct syntax_top_module *)malloc(sizeof(struct syntax_top_module));
      module->type = STATEMENT;
      module->data.statement = stmt;
      program->modules->push(program->modules, module);
    }
  }
  return program;
}

void parser_type_def(struct Parser *parser) {
  struct type_def_meta_data *meta_data = new_type_def_meta_data();
  expected_token_type_and_run(parser->sc, TOKEN_TYPE_DEF);
  struct token n = parser->sc->cur_token;
  expected_token_type_and_run(parser->sc, TOKEN_ID);
  meta_data->type_name =
      parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index)->str;
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_BRACE);
  while (parser->sc->cur_token.type != TOKEN_RIGHT_BRACE) {
    const char *key =
        parser->sc->symbol
            ->get(parser->sc->symbol, parser->sc->cur_token.value.symbol_index)
            ->str;
    expected_token_type_and_run(parser->sc, TOKEN_ID);
    expected_token_type_and_run(parser->sc, TOKEN_COLON);
    const char *value =
        parser->sc->symbol
            ->get(parser->sc->symbol, parser->sc->cur_token.value.symbol_index)
            ->str;
    struct zero_type *t = get_zero_type(value);
    expected_token_type_and_run(parser->sc, TOKEN_ID);
    map_insert(meta_data->segments, key, t);
    expected_token_type_and_run(parser->sc, TOKEN_SEMICOLON);
  }
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_BRACE);
  expected_token_type_and_run(parser->sc, TOKEN_SEMICOLON);
  map_insert(parser->cur_scope->type_defs, meta_data->type_name, meta_data);
}

void parser_function_define_params_helper(struct syntax_function_define *fd,
                                          struct Parser *parser) {
  while (parser->sc->cur_token.type != TOKEN_RIGHT_PARENT) {
    struct token n = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, TOKEN_ID);
    soft_expected_token_type_and_run(parser->sc, TOKEN_COMMA);
    const char *p =
        parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index)->str;
    map_insert(fd->params, p, (void *)fd->params->count);
    map_insert(parser->cur_scope->params, p,
               (void *)parser->cur_scope->params->count);
  }
}

void parser_function_define_statements_helper(struct syntax_function_define *fd,
                                              struct Parser *parser) {
  while (parser->sc->cur_token.type != TOKEN_RIGHT_BRACE) {
    struct syntax_statement *stmt = parser_statement(parser);
    fd->statements->push(fd->statements, stmt);
  }
}

struct syntax_function_define *parser_function_define(struct Parser *parser) {
  expected_token_type_and_run(parser->sc, TOKEN_FUNC);
  struct token n = parser->sc->cur_token;
  expected_token_type_and_run(parser->sc, TOKEN_ID);
  const char *func_name =
      parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index)->str;
  struct syntax_function_define *fd = new_function_define(func_name);
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_PARENT);
  parser_function_define_params_helper(fd, parser);
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_PARENT);
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_BRACE);
  parser_function_define_statements_helper(fd, parser);
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_BRACE);
  return fd;
}

struct syntax_statement *parser_include_statement(struct Parser *parser) {
  expected_token_type_and_run(parser->sc, TOKEN_INCLUDE);
  struct syntax_statement *include_stmt =
      (struct syntax_statement *)malloc(sizeof(struct syntax_statement));
  include_stmt->type = STMT_INCLUDE;
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_PARENT);
  struct token n = parser->sc->cur_token;
  expected_token_type_and_run(parser->sc, TOKEN_STRING);
  include_stmt->data.include_stmt.include_path =
      parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index)->str;
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_PARENT);
  expected_token_type_and_run(parser->sc, TOKEN_SEMICOLON);
  return include_stmt;
}

struct syntax_statement *parser_statement(struct Parser *parser) {
  struct token cur_token = parser->sc->cur_token;
  if (cur_token.type == TOKEN_KEYWORD &&
      strcmp(parser->sc->symbol
                 ->get(parser->sc->symbol, cur_token.value.symbol_index)
                 ->str,
             "var") == 0) {
    return parser_var_decl_stmt(parser);
  }
  if (cur_token.type == TOKEN_RETURN) {
    return parser_ret_stmt(parser);
  }
  if (cur_token.type == TOKEN_IF) {
    return parser_if_statement(parser);
  }
  if (cur_token.type == TOKEN_LEFT_BRACE) {
    return parser_block_statement(parser);
  }
  if (cur_token.type == TOKEN_WHILE) {
    return parser_while_statement(parser);
  }
  if (cur_token.type == TOKEN_INCLUDE) {
    return parser_include_statement(parser);
  }
  return parser_expr_stmt(parser);
}

struct syntax_statement *parser_while_statement(struct Parser *parser) {
  expected_token_type_and_run(parser->sc, TOKEN_WHILE);
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_PARENT);
  struct syntax_expr *expression = parser_expr(parser);
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_PARENT);
  struct syntax_statement *while_block = parser_block_statement(parser);

  struct syntax_statement *while_stmt =
      (struct syntax_statement *)malloc(sizeof(struct syntax_statement));
  while_stmt->type = STMT_WHILE;
  while_stmt->data.while_stmt.expression = expression;
  while_stmt->data.while_stmt.while_block = while_block;
  return while_stmt;
}

struct syntax_statement *parser_if_statement(struct Parser *parser) {
  expected_token_type_and_run(parser->sc, TOKEN_IF);
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_PARENT);
  struct syntax_expr *expression = parser_expr(parser);
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_PARENT);
  struct syntax_statement *if_block = parser_block_statement(parser);
  struct syntax_statement *if_stmt =
      (struct syntax_statement *)malloc(sizeof(struct syntax_statement));
  if_stmt->type = STMT_IF;
  if_stmt->data.if_stmt.if_block = if_block;
  if_stmt->data.if_stmt.expression = expression;
  if (parser->sc->cur_token.type == TOKEN_ELSE) {
    expected_token_type_and_run(parser->sc, TOKEN_ELSE);
    if (parser->sc->cur_token.type == TOKEN_IF) {
      if_stmt->data.if_stmt.else_block = parser_if_statement(parser);
    } else if (parser->sc->cur_token.type == TOKEN_LEFT_BRACE) {
      if_stmt->data.if_stmt.else_block = parser_block_statement(parser);
    } else {
      assert(false);
    }
  } else {
    if_stmt->data.if_stmt.else_block = NULL;
  }
  return if_stmt;
}

struct syntax_statement *parser_block_statement(struct Parser *parser) {
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_BRACE);
  struct parser_scope *psp = new_parser_scope();
  psp->parent = parser->cur_scope;
  psp->params = parser->cur_scope->params;
  parser->cur_scope = psp;
  struct syntax_statement *b_stmt =
      (struct syntax_statement *)malloc(sizeof(struct syntax_statement));
  b_stmt->data.block_stmt.statements = new_vec();
  b_stmt->type = STMT_BLOCK;
  while (parser->sc->cur_token.type != TOKEN_RIGHT_BRACE) {
    struct syntax_statement *st = parser_statement(parser);
    b_stmt->data.block_stmt.statements->push(b_stmt->data.block_stmt.statements,
                                             st);
  }
  b_stmt->data.block_stmt.var_count = parser->cur_scope->variables->count;
  parser->cur_scope = parser->cur_scope->parent;
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_BRACE);
  return b_stmt;
}

struct syntax_statement *parser_ret_stmt(struct Parser *parser) {
  expected_token_type_and_run(parser->sc, TOKEN_RETURN);
  struct syntax_expr *expr = parser_expr(parser);
  struct syntax_statement *statement =
      (struct syntax_statement *)malloc(sizeof(struct syntax_statement));
  statement->type = STMT_RETURN;
  statement->data.ret_stmt.expr = expr;
  expected_token_type_and_run(parser->sc, TOKEN_SEMICOLON);
  return statement;
}

int get_var_index(struct parser_scope *scope) {
  int index = 0;
  while (scope != NULL && !scope->is_top_scope) {
    index += scope->variables->count;
    scope = scope->parent;
  }
  return index;
}

struct syntax_statement *parser_var_decl_stmt(struct Parser *parser) {
  expected_token_type_str_value_and_run(parser->sc, TOKEN_KEYWORD, "var");
  char *id_name =
      parser->sc->symbol
          ->get(parser->sc->symbol, parser->sc->cur_token.value.symbol_index)
          ->str;
  struct syntax_statement *statement =
      (struct syntax_statement *)malloc(sizeof(struct syntax_statement));
  statement->type = STMT_VAR_DECL;
  expected_token_type_and_run(parser->sc, TOKEN_ID);
  if (parser->sc->cur_token.type == TOKEN_ASSIGN) {
    expected_token_type_and_run(parser->sc, TOKEN_ASSIGN);
    struct syntax_expr *expr = parser_expr(parser);
    statement->data.var_stmt.initializer = expr;
  } else {
    struct syntax_expr *null =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    null->type = EXPR_LITERAL;
    null->data.literal_expr.kind = LIT_NULL;
    statement->data.var_stmt.initializer = null;
  }
  statement->data.var_stmt.name = id_name;
  if (parser->cur_scope->is_top_scope) {
    if (map_get(parser->cur_scope->variables, id_name) == NULL) {
      statement->data.var_stmt.var_index = parser->root_scope->top_var_count++;
      map_insert(parser->cur_scope->variables, id_name,
                 (void *)statement->data.var_stmt.var_index);
      // printf("insert %s  in %d\n", id_name,
      // statement->data.var_stmt.var_index);
    } else {
      printf("variable(%s) redefine\n", id_name);
      assert(false);
    }
  } else {
    if (map_get(parser->cur_scope->variables, id_name) == NULL &&
        map_get(parser->cur_scope->params, id_name) == NULL) {
      statement->data.var_stmt.var_index = get_var_index(parser->cur_scope);
      int index = statement->data.var_stmt.var_index;
      map_insert(parser->cur_scope->variables, id_name, (void *)index);
    } else {
      printf("variable(%s) redefine\n", id_name);
      assert(false);
    }
  }

  expected_token_type_and_run(parser->sc, TOKEN_SEMICOLON);
  return statement;
}

struct syntax_statement *parser_expr_stmt(struct Parser *parser) {
  struct syntax_statement *statement =
      (struct syntax_statement *)malloc(sizeof(struct syntax_statement));
  statement->type = STMT_EXPR;
  statement->data.expr_stmt.expr = parser_expr(parser);
  if (!(statement->data.expr_stmt.expr->type == EXPR_BINARY &&
        statement->data.expr_stmt.expr->data.binary_expr.op == TOKEN_ASSIGN)) {
    statement->data.expr_stmt.need_pop = true;
  } else {
    statement->data.expr_stmt.need_pop = false;
  }
  expected_token_type_and_run(parser->sc, TOKEN_SEMICOLON);
  return statement;
}
struct syntax_expr *parser_expr(struct Parser *parser) {
  if (parser->sc->cur_token.type == TOKEN_NEW) {
    return parser_new(parser);
  } else {
    struct syntax_expr *id = parser_logic_or(parser);
    if (parser->sc->cur_token.type == TOKEN_ASSIGN) {
      expected_token_type_and_run(parser->sc, TOKEN_ASSIGN);
      struct syntax_expr *binary =
          (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
      binary->type = EXPR_BINARY;
      binary->data.binary_expr.left = id;
      binary->data.binary_expr.right = parser_expr(parser);
      binary->data.binary_expr.op = TOKEN_ASSIGN;
      return binary;
    }
    return id;
  }
}

MapPair *get_type_def_meta_data(struct parser_scope *scope, const char *name) {
  while (scope != NULL) {
    MapPair *pair = map_get(scope->type_defs, name);
    if (pair != NULL) {
      return pair;
    }
    scope = scope->parent;
  }
  return NULL;
}

struct syntax_expr *parser_new(struct Parser *parser) {
  expected_token_type_and_run(parser->sc, TOKEN_NEW);
  struct token n = parser->sc->cur_token;
  expected_token_type_and_run(parser->sc, TOKEN_ID);
  const char *type_name =
      parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index)->str;
  MapPair *pair = get_type_def_meta_data(parser->cur_scope, type_name);
  assert(pair != NULL);
  struct syntax_expr *expr =
      (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
  expr->type = EXPR_NEW;
  expr->data.new_expr.mete_data = (struct type_def_meta_data *)(pair->value);
  expr->data.new_expr.init_list = new_map();
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_BRACE);
  while (parser->sc->cur_token.type != TOKEN_RIGHT_BRACE) {
    struct token n = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, TOKEN_ID);
    const char *key =
        parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index)->str;
    expected_token_type_and_run(parser->sc, TOKEN_ASSIGN);
    struct syntax_expr *init_expr = parser_expr(parser);
    soft_expected_token_type_and_run(parser->sc, TOKEN_COMMA);
    map_insert(expr->data.new_expr.init_list, key, init_expr);
  }
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_BRACE);
  return expr;
}

struct syntax_expr *parser_assignment_expr(struct Parser *parser) {
  struct syntax_expr *id = parser_primary(parser);
  expected_token_type_and_run(parser->sc, TOKEN_ASSIGN);
  struct syntax_expr *binary =
      (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
  binary->type = EXPR_BINARY;
  binary->data.binary_expr.left = id;
  binary->data.binary_expr.right = parser_expr(parser);
  binary->data.binary_expr.op = TOKEN_ASSIGN;
  return binary;
}

struct syntax_expr *parser_logic_or(struct Parser *parser) {
  struct syntax_expr *left = parser_logic_and(parser);
  while (parser->sc->cur_token.type == TOKEN_OR) {
    struct token op = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, parser->sc->cur_token.type);
    struct syntax_expr *right = parser_logic_and(parser);
    struct syntax_expr *binary =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    binary->type = EXPR_BINARY;
    binary->data.binary_expr.left = left;
    binary->data.binary_expr.right = right;
    binary->data.binary_expr.op = op.type;
    left = binary;
  }
  return left;
}

struct syntax_expr *parser_logic_and(struct Parser *parser) {
  struct syntax_expr *left = parser_equality(parser);
  while (parser->sc->cur_token.type == TOKEN_AND) {
    struct token op = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, parser->sc->cur_token.type);
    struct syntax_expr *right = parser_equality(parser);
    struct syntax_expr *binary =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    binary->type = EXPR_BINARY;
    binary->data.binary_expr.left = left;
    binary->data.binary_expr.right = right;
    binary->data.binary_expr.op = op.type;
    left = binary;
  }
  return left;
}

struct syntax_expr *parser_equality(struct Parser *parser) {
  struct syntax_expr *left = parser_comparison(parser);
  while (parser->sc->cur_token.type == TOKEN_EQUAL ||
         parser->sc->cur_token.type == TOKEN_NOT_EQUAL) {
    struct token op = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, parser->sc->cur_token.type);
    struct syntax_expr *right = parser_comparison(parser);
    struct syntax_expr *binary =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    binary->type = EXPR_BINARY;
    binary->data.binary_expr.left = left;
    binary->data.binary_expr.right = right;
    binary->data.binary_expr.op = op.type;
    left = binary;
  }
  return left;
}

struct syntax_expr *parser_comparison(struct Parser *parser) {
  struct syntax_expr *left = parser_assignment(parser);
  while (parser->sc->cur_token.type == TOKEN_GREATER ||
         parser->sc->cur_token.type == TOKEN_GREATER_EQUAL ||
         parser->sc->cur_token.type == TOKEN_LESS ||
         parser->sc->cur_token.type == TOKEN_LESS_EQUAL) {
    struct token op = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, parser->sc->cur_token.type);
    struct syntax_expr *right = parser_assignment(parser);
    struct syntax_expr *binary =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    binary->type = EXPR_BINARY;
    binary->data.binary_expr.left = left;
    binary->data.binary_expr.right = right;
    binary->data.binary_expr.op = op.type;
    left = binary;
  }
  return left;
}

struct syntax_expr *parser_assignment(struct Parser *parser) {
  struct syntax_expr *term = parser_term(parser);
  while (parser->sc->cur_token.type == TOKEN_ADD ||
         parser->sc->cur_token.type == TOKEN_MINUS) {
    struct token op = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, parser->sc->cur_token.type);
    struct syntax_expr *right = parser_term(parser);
    struct syntax_expr *binary =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    binary->type = EXPR_BINARY;
    binary->data.binary_expr.left = term;
    binary->data.binary_expr.right = right;
    binary->data.binary_expr.op = op.type;
    term = binary;
  }
  return term;
}

struct syntax_expr *parser_term(struct Parser *parser) {
  struct syntax_expr *node = parser_factory(parser);
  while (parser->sc->cur_token.type == TOKEN_MULTI ||
         parser->sc->cur_token.type == TOKEN_DIV) {
    struct token op = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, parser->sc->cur_token.type);
    struct syntax_expr *right = parser_factory(parser);
    struct syntax_expr *binary =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    binary->type = EXPR_BINARY;
    binary->data.binary_expr.left = node;
    binary->data.binary_expr.right = right;
    binary->data.binary_expr.op = op.type;
    node = binary;
  }
  return node;
}

struct syntax_expr *parser_factory(struct Parser *parser) {
  struct token n = parser->sc->cur_token;
  bool is_unary = false;
  if (n.type == TOKEN_MINUS || n.type == TOKEN_NOT) {
    is_unary = true;
    expected_token_type_and_run(parser->sc, parser->sc->cur_token.type);
  }
  struct syntax_expr *access = parser_access(parser);
  if (is_unary) {
    struct syntax_expr *unary =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    unary->type = EXPR_UNARY;
    unary->data.binary_expr.op = n.type;
    unary->data.binary_expr.right = access;
    unary->data.binary_expr.left = NULL;
    access = unary;
  }
  return access;
}

struct syntax_expr *parser_access(struct Parser *parser) {
  struct syntax_expr *primary_call = parser_primary_call(parser);
  while (parser->sc->cur_token.type == TOKEN_ACCESS) {
    expected_token_type_and_run(parser->sc, TOKEN_ACCESS);
    struct syntax_expr *access =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    access->type = EXPR_ACCESS;
    access->data.access_expr.obj = primary_call;
    struct syntax_expr *prop = parser_primary_call(parser);
    if (prop->type == EXPR_ID) {
      prop->data.identifier_expr.is_in_access = true;
    } else if (prop->type == EXPR_CALL) {
      prop->data.call_expr.source_in_stack = true;
    }
    access->data.access_expr.prop = prop;
    primary_call = access;
  }
  return primary_call;
}

struct syntax_expr *parser_primary_call(struct Parser *parser) {
  struct syntax_expr *primary = parser_primary(parser);
  if (parser->sc->cur_token.type == TOKEN_LEFT_PARENT) {
    primary = parser_call(primary, parser);
  }
  return primary;
}

// just for debug
void symbol_print(struct str_store *store) {
  struct str_item *item = store->head;
  int index = 1;
  while (item != NULL) {
    printf("[%d]: %s\n", index++, item->str);
    item = item->next;
  }
}

MapPair *get_variables_from_parser_scope(struct parser_scope *scope,
                                         const char *key) {
  while (scope != NULL) {
    MapPair *pair = map_get(scope->variables, key);
    if (pair != NULL) {
      return pair;
    }
    scope = scope->parent;
  }
  return NULL;
}

int get_var_index_in_scope(struct parser_scope *scope, const char *key) {
  while (scope != NULL) {
    MapPair *p = map_get(scope->variables, key);
    if (p != NULL) {
      return (int)p->value;
    }
    scope = scope->parent;
  }
}

bool get_var_in_scope_without_top(struct parser_scope *scope, const char *key) {
  while (scope != NULL && !scope->is_top_scope) {
    MapPair *p = map_get(scope->variables, key);
    if (p != NULL) {
      return true;
    }
    scope = scope->parent;
  }
  return false;
}

struct syntax_expr *parser_primary(struct Parser *parser) {
  switch (parser->sc->cur_token.type) {
  case TOKEN_NUM: {
    struct token n = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, TOKEN_NUM);
    struct syntax_expr *number =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    number->type = EXPR_LITERAL;
    if (n.value.number_value.type == NUMBER_INT) {
      number->data.literal_expr.kind = LIT_INT;
      number->data.literal_expr.int_val = n.value.number_value.int_value;
    } else if (n.value.number_value.type == NUMBER_FLOAT) {
      number->data.literal_expr.kind = LIT_FLOAT;
      number->data.literal_expr.float_val = n.value.number_value.float_value;
    } else if (n.value.number_value.type == NUMBER_BOOL) {
      number->data.literal_expr.kind = LIT_BOOL;
      number->data.literal_expr.bool_val = n.value.number_value.bool_value;
    }
    return number;
  }
  case TOKEN_NULL: {
    expected_token_type_and_run(parser->sc, TOKEN_NULL);
    struct syntax_expr *null =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    null->type = EXPR_LITERAL;
    null->data.literal_expr.kind = LIT_NULL;
    return null;
  }
  case TOKEN_STRING: {
    struct token n = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, TOKEN_STRING);
    struct syntax_expr *str =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    str->type = EXPR_LITERAL;
    str->data.literal_expr.kind = LIT_STR;
    str->data.literal_expr.str_val =
        parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index)->str;
    return str;
  }
  case TOKEN_ID:
  case TOKEN_THIS: {
    struct token n = parser->sc->cur_token;
    bool is_this = parser->sc->cur_token.type == TOKEN_THIS;
    if (is_this) {
      expected_token_type_and_run(parser->sc, TOKEN_THIS);
    } else {
      expected_token_type_and_run(parser->sc, TOKEN_ID);
    }
    struct syntax_expr *id =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    id->type = EXPR_ID;
    id->data.identifier_expr.is_this = is_this;
    id->data.identifier_expr.is_in_access = false;
    if (!is_this) {
      id->data.identifier_expr.may_be_function = false;
      id->data.identifier_expr.from_params = false;
      id->data.identifier_expr.name =
          parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index)
              ->str;
      MapPair *variable_define_form_var = get_variables_from_parser_scope(
          parser->cur_scope, id->data.identifier_expr.name);
      MapPair *variable_define_from_params =
          map_get(parser->cur_scope->params, id->data.identifier_expr.name);
      if (variable_define_from_params == NULL &&
          variable_define_form_var == NULL &&
          parser->sc->cur_token.type != TOKEN_LEFT_PARENT) {
        // printf("Variable(%s) not defined, but may be a function?\n",
        //        id->data.identifier_expr.name);
        id->data.identifier_expr.may_be_function = true;
        return id;
      }
      if (variable_define_from_params != NULL) {
        id->data.identifier_expr.from_params = true;
        id->data.identifier_expr.offset =
            (int)variable_define_from_params->value;
      } else {
        id->data.identifier_expr.is_from_top = !get_var_in_scope_without_top(
            parser->cur_scope, id->data.identifier_expr.name);

        id->data.identifier_expr.var_index = get_var_index_in_scope(
            parser->cur_scope, id->data.identifier_expr.name);
      }
    }

    if (is_this) {
      id->data.identifier_expr.offset = parser->cur_scope->params->count;
    }
    return id;
  }
  case TOKEN_LEFT_BRACKET: // [
    return parser_arraylist(parser);
  case TOKEN_LEFT_BRACE: // {
    return parser_objectlist(parser);
  case TOKEN_LEFT_PARENT: {
    expected_token_type_and_run(parser->sc, TOKEN_LEFT_PARENT);
    struct syntax_expr *expr = parser_expr(parser);
    expected_token_type_and_run(parser->sc, TOKEN_RIGHT_PARENT);
    return expr;
  }
  default:
    return NULL;
  }
}

struct syntax_expr *parser_call(struct syntax_expr *caller,
                                struct Parser *parser) {
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_PARENT);
  struct Vec *args = new_vec();
  while (parser->sc->cur_token.type != TOKEN_RIGHT_PARENT) {
    args->push(args, parser_expr(parser));
    soft_expected_token_type_and_run(parser->sc, TOKEN_COMMA);
  }
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_PARENT);
  struct syntax_expr *call =
      (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
  call->type = EXPR_CALL;
  call->data.call_expr.source_in_stack = false;
  if (caller->type == EXPR_ID) {
    call->data.call_expr.func_name = caller->data.identifier_expr.name;
    call->data.call_expr.is_from_params =
        caller->data.identifier_expr.from_params;
    call->data.call_expr.offset = caller->data.identifier_expr.offset;
    if (!caller->data.identifier_expr.is_from_top) {
      call->data.call_expr.var_index = caller->data.identifier_expr.var_index;
    } else {
      call->data.call_expr.var_index = -1;
    }
  }
  call->data.call_expr.args = args;
  return call;
}

struct syntax_expr *parser_arraylist(struct Parser *parser) {
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_BRACKET);
  struct syntax_expr *expr =
      (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
  expr->type = EXPR_ARRAY;
  expr->data.array_expr.elements = new_vec();
  if (parser->sc->cur_token.type != TOKEN_RIGHT_BRACKET) {
    do {
      if (parser->sc->cur_token.type == TOKEN_COMMA) {
        scanner_run(parser->sc);
      }
      expr->data.array_expr.elements->push(expr->data.array_expr.elements,
                                           parser_expr(parser));
    } while (parser->sc->cur_token.type == TOKEN_COMMA);
  }
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_BRACKET);
  return expr;
}

struct syntax_expr *parser_objectlist(struct Parser *parser) {
  expected_token_type_and_run(parser->sc, TOKEN_LEFT_BRACE);
  struct syntax_expr *expr =
      (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
  expr->type = EXPR_OBJECT;
  expr->data.object_expr.pairs = new_vec();
  do {
    if (parser->sc->cur_token.type == TOKEN_COMMA) {
      scanner_run(parser->sc);
    }
    struct token key = parser->sc->cur_token;
    struct syntax_kv_pair *pair =
        (struct syntax_kv_pair *)malloc(sizeof(struct syntax_kv_pair));
    pair->key =
        parser->sc->symbol->get(parser->sc->symbol, key.value.symbol_index)
            ->str;
    expected_token_type_and_run(parser->sc, TOKEN_STRING);
    expected_token_type_and_run(parser->sc, TOKEN_COLON);
    pair->value = parser_expr(parser);
    expr->data.object_expr.pairs->push(expr->data.object_expr.pairs, pair);
  } while (parser->sc->cur_token.type == TOKEN_COMMA);
  expected_token_type_and_run(parser->sc, TOKEN_RIGHT_BRACE);
  return expr;
}

int INSTRUCTION_SAVE(const char *seg, const char *fmt, ...);
void INSTRUCTION_UPDATE(const char *seg, int index, const char *fmt, ...);
int INSTRUCTION_COUNT(const char *seg);

// parser visitor functions
void function_define_visitor(struct syntax_function_define *fd);
void statement_visitor(struct syntax_statement *statment);
void statement_stmt_var_decl_visitor(struct syntax_statement *statement);
void statement_stmt_expr_visitor(struct syntax_statement *statement);
void statement_stmt_ret_visitor(struct syntax_statement *statement);
void statement_stmt_if_visitor(struct syntax_statement *statement);
void statement_stmt_while_visitor(struct syntax_statement *statement);
void statement_stmt_block_visitor(struct syntax_statement *statement);
void expression_visitor(struct syntax_expr *expr);
void expression_literal_visitor(struct syntax_expr *expr);
void expression_binary_visitor(struct syntax_expr *expr);
void expression_unary_visitor(struct syntax_expr *expr);
void expression_array_visitor(struct syntax_expr *expr);
void expression_object_visitor(struct syntax_expr *expr);
void expression_access_visitor(struct syntax_expr *expr);
void expression_access_visitor_for_call(struct syntax_expr *expr);
void expression_new_visitor(struct syntax_expr *expr);
void expression_call_visitor(struct syntax_expr *expr);
void program_visitor(struct syntax_program *program);

char *CURRENT_FUNCTION_NAME = NULL;

void program_visitor(struct syntax_program *program) {
  for (int i = 0; i < program->modules->count; i++) {
    struct syntax_top_module *module =
        (struct syntax_top_module *)program->modules->get(program->modules, i);
    if (module->type == STATEMENT) {
      CURRENT_FUNCTION_NAME = "__init__";
      statement_visitor(module->data.statement);
    } else if (module->type == FUNC_DEFINE) {
      function_define_visitor(module->data.func_define);
    }
  }
}

void function_define_visitor(struct syntax_function_define *fd) {
  CURRENT_FUNCTION_NAME = fd->name;
  for (int i = 0; i < fd->statements->count; i++) {
    struct syntax_statement *statement =
        (struct syntax_top_module *)fd->statements->get(fd->statements, i);
    statement_visitor(statement);
  }
}

void statement_visitor(struct syntax_statement *statment) {
  switch (statment->type) {
  case STMT_VAR_DECL:
    return statement_stmt_var_decl_visitor(statment);
  case STMT_EXPR: {
    statement_stmt_expr_visitor(statment);
    if (statment->data.expr_stmt.need_pop) {
      INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "FREE 1");
    }
    return;
  }
  case STMT_RETURN:
    return statement_stmt_ret_visitor(statment);
  case STMT_IF:
    return statement_stmt_if_visitor(statment);
  case STMT_BLOCK:
    return statement_stmt_block_visitor(statment);
  case STMT_WHILE:
    return statement_stmt_while_visitor(statment);
  default:
    assert(false);
    break;
  }
}

/*
      [4] ... <--------------------------------|
      [5] ... enter while                      |
      [6] ...                                  |
------[7] exit_loop_index 4 (11 - 7)           |
|     [8] ...                                  |
|     [9] ...                                  |
|     [10] ...                                 |
|---->[11] return_loop_index -7 (4 - 11)--------
*/
void statement_stmt_while_visitor(struct syntax_statement *statement) {
  int count = INSTRUCTION_COUNT(CURRENT_FUNCTION_NAME);
  expression_visitor(statement->data.while_stmt.expression);
  int exit_loop_index = INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, NULL);
  statement_visitor(statement->data.while_stmt.while_block);
  int return_loop_index = INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, NULL);
  INSTRUCTION_UPDATE(CURRENT_FUNCTION_NAME, exit_loop_index, "JF %d",
                     return_loop_index - exit_loop_index);
  INSTRUCTION_UPDATE(CURRENT_FUNCTION_NAME, return_loop_index, "JUMP %d",
                     count - return_loop_index);
}

/*
      [4] ...
      [5] ... if
      [6] ...
------[7] jump_to_false 4 (11 - 7)
|     [8] ...
|     [9] ...
|     [10] exit ------------
|---->[11] else            |
      [12]                 |
      [13]                 |
      [14]<----------------|
      [15]
*/
void statement_stmt_if_visitor(struct syntax_statement *statement) {
  expression_visitor(statement->data.if_stmt.expression);
  int jump_to_false_index = INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, NULL);
  statement_visitor(statement->data.if_stmt.if_block);
  int jump_to_end_index;
  if (statement->data.if_stmt.else_block != NULL) {
    jump_to_end_index = INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, NULL);
  }
  int count = INSTRUCTION_COUNT(CURRENT_FUNCTION_NAME);
  INSTRUCTION_UPDATE(CURRENT_FUNCTION_NAME, jump_to_false_index, "JF %d",
                     count - jump_to_false_index - 1);
  if (statement->data.if_stmt.else_block != NULL) {
    statement_visitor(statement->data.if_stmt.else_block);
    count = INSTRUCTION_COUNT(CURRENT_FUNCTION_NAME);
    INSTRUCTION_UPDATE(CURRENT_FUNCTION_NAME, jump_to_end_index, "JUMP %d",
                       count - jump_to_end_index - 1);
  }
}

void statement_stmt_block_visitor(struct syntax_statement *statement) {
  for (int i = 0; i < statement->data.block_stmt.statements->count; i++) {
    statement_visitor(statement->data.block_stmt.statements->get(
        statement->data.block_stmt.statements, i));
  }
  if (statement->data.block_stmt.var_count > 0) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "FREE %d",
                     statement->data.block_stmt.var_count);
  }
}

void statement_stmt_ret_visitor(struct syntax_statement *statement) {
  expression_visitor(statement->data.ret_stmt.expr);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "STORE [ei]",
                   statement->data.var_stmt.name);
}

struct instruction_store {
  int fd;
  Map *map;
  struct Vec *childs;
  struct instruction_store *parent;
  struct str_store *static_func;
};

struct instruction_store *root_instruction_store = NULL;

struct instruction_store *cur_instruction_store = NULL;

struct instruction_store *GET_INSTRUCTION_STORE() {
  // if (__instruction_store == NULL) {
  //   __instruction_store =
  //       (struct instruction_store *)malloc(sizeof(struct instruction_store));
  //   __instruction_store->fd = -1;
  //   __instruction_store->map = new_map();
  // }
  return cur_instruction_store;
}

struct instruction_store *new_instruction_store() {
  struct instruction_store *inst_store =
      (struct instruction_store *)malloc(sizeof(struct instruction_store));
  inst_store->fd = -1;
  inst_store->map = new_map();
  inst_store->childs = new_vec();
  inst_store->parent = NULL;
  struct str_store *s = str_store_init();
  inst_store->static_func = s;
  return inst_store;
}

void ROOT_INSTRUCTION_INIT() {
  root_instruction_store =
      (struct instruction_store *)malloc(sizeof(struct instruction_store));
  root_instruction_store->fd = -1;
  root_instruction_store->map = new_map();
  root_instruction_store->childs = new_vec();
  root_instruction_store->parent = NULL;
  struct str_store *s = str_store_init();
  root_instruction_store->static_func = s;
}

void INSTRUCTION_UPDATE(const char *seg, int index, const char *fmt, ...) {
  struct instruction_store *gs = GET_INSTRUCTION_STORE();
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  if (len < 0)
    return;
  char *buf = malloc(len + 1);
  va_start(args, fmt);
  vsnprintf(buf, len + 1, fmt, args);
  va_end(args);
  if (strcmp(seg, "__init__") == 0) {
    gs->static_func->get(gs->static_func, index)->str = buf;
  } else {
    struct map_pair *pair = map_get(gs->map, seg);
    if (pair == NULL) {
      return;
    }
    struct str_store *s = (struct str_store *)pair->value;
    s->get(s, index)->str = buf;
  }
}

int INSTRUCTION_COUNT(const char *seg) {
  struct instruction_store *gs = GET_INSTRUCTION_STORE();
  if (strcmp(seg, "__init__") == 0) {
    return gs->static_func->count;
  } else {
    struct map_pair *pair = map_get(gs->map, seg);
    if (pair == NULL) {
      return 0; // if have no instruction, count is 0. okay?
    }
    struct str_store *s = (struct str_store *)pair->value;
    return s->count;
  }
}

void INSTRUCTION_REMOVE_END(const char *seg) {
  struct instruction_store *gs = GET_INSTRUCTION_STORE();
  if (strcmp(seg, "__init__") == 0) {
    gs->static_func->remove_end(gs->static_func);
  } else {
    struct map_pair *pair = map_get(gs->map, seg);
    if (pair == NULL) {
      return;
    }
    struct str_store *s = (struct str_store *)pair->value;
    s->remove_end(s);
  }
}

int INSTRUCTION_SAVE(const char *seg, const char *fmt, ...) {
  struct instruction_store *gs = GET_INSTRUCTION_STORE();
  char *buf = NULL;
  if (fmt != NULL) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (len < 0)
      return -1;
    buf = malloc(len + 1);
    va_start(args, fmt);
    vsnprintf(buf, len + 1, fmt, args);
    va_end(args);
  }
  // if (gs->fd != -1) {
  //   if (len > 0) {
  //     write(gs->fd, buf, len);
  //     write(gs->fd, "\n", 1);
  //   }
  // }
  if (strcmp(seg, "__init__") == 0) {
    gs->static_func->insert_raw(gs->static_func, buf);
  } else {
    struct map_pair *pair = map_get(gs->map, seg);
    if (pair == NULL) {
      struct str_store *s = str_store_init();
      s->insert_raw(s, buf);
      map_insert(gs->map, seg, s);
      return 0;
    }
    struct str_store *s = (struct str_store *)pair->value;
    return s->insert_raw(s, buf);
  }
}

void itf(MapPair *pair, void *) {
  int fd = GET_INSTRUCTION_STORE()->fd;
  write(fd, pair->key, strlen(pair->key));
  write(fd, ":\n", 2);
  struct str_store *store = (struct str_store *)pair->value;
  for (int i = 0; i < store->count; i++) {
    const char *inst = store->get(store, i)->str;
    write(fd, "  ", 2);
    write(fd, inst, strlen(inst));
    write(fd, "\n", 1);
  }
}

void instruction_to_file() {
  if (GET_INSTRUCTION_STORE()->static_func != NULL) {
    MapPair pair = (MapPair){.key = "__init__",
                             .value = GET_INSTRUCTION_STORE()->static_func};
    itf(&pair, NULL);
  }
  map_foreach(GET_INSTRUCTION_STORE()->map, itf, NULL);
}

void statement_stmt_var_decl_visitor(struct syntax_statement *statement) {
  assert(statement->type == STMT_VAR_DECL);
  expression_visitor(statement->data.var_stmt.initializer);
  if (strcmp(CURRENT_FUNCTION_NAME, "__init__") == 0) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "STORE %d",
                     statement->data.var_stmt.var_index);
  } else {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "STORE #%d",
                     statement->data.var_stmt.var_index + 1);
  }
}
void statement_stmt_expr_visitor(struct syntax_statement *statement) {
  assert(statement->type == STMT_EXPR);
  expression_visitor(statement->data.expr_stmt.expr);
}

void expression_visitor(struct syntax_expr *expr) {
  switch (expr->type) {
  case EXPR_ID: {
    if (expr->data.identifier_expr.is_this) {
      INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD #-%d", // bp-%d
                       expr->data.identifier_expr.offset + 1);
    } else {
      if (expr->data.identifier_expr.is_in_access) {
        INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_S %s", // function name
                         expr->data.identifier_expr.name);
      } else {
        if (expr->data.identifier_expr.may_be_function) {
          INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD .%s", // function name
                           expr->data.identifier_expr.name);
        } else {
          if (expr->data.identifier_expr.from_params) {
            INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD #-%d", // bp-%d
                             expr->data.identifier_expr.offset);
          } else {
            if (expr->data.identifier_expr.is_from_top) {
              INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD %d", // base+%d
                               expr->data.identifier_expr.var_index);
            } else {
              INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD #%d",
                               expr->data.identifier_expr.var_index +
                                   1); // bp+d
            }
          }
        }
      }
    }

  } break;
  case EXPR_BINARY:
    expression_binary_visitor(expr);
    break;
  case EXPR_UNARY:
    expression_unary_visitor(expr);
    break;
  case EXPR_LITERAL:
    expression_literal_visitor(expr);
    break;
  case EXPR_CALL:
    expression_call_visitor(expr);
    break;
  case EXPR_ARRAY:
    expression_array_visitor(expr);
    break;
  case EXPR_OBJECT:
    expression_object_visitor(expr);
    break;
  case EXPR_ACCESS:
    expression_access_visitor(expr);
    break;
  case EXPR_NEW:
    expression_new_visitor(expr);
    break;
  default:
    assert(false);
  }
}

void expression_new_visitor_helper(MapPair *pair, void *data) {
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_S %s", pair->key);
  struct syntax_expr *expr = (struct syntax_expr *)data;
  MapPair *init_pair = map_get(expr->data.new_expr.init_list, pair->key);
  if (init_pair != NULL) {
    expression_visitor(init_pair->value);
    struct zero_type *t = (struct zero_type *)pair->value;
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CHECK %d", t->type);
  } else {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_N");
  }
}

void expression_new_visitor(struct syntax_expr *expr) {
  assert(expr->type == EXPR_NEW);
  map_foreach(expr->data.new_expr.mete_data->segments,
              expression_new_visitor_helper, expr);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_D %d",
                   expr->data.new_expr.mete_data->segments->count);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL native_new_object");
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "FREE %d",
                   expr->data.new_expr.mete_data->segments->count * 2 + 1);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD [ei]");
}

void expression_access_visitor(struct syntax_expr *expr) {
  assert(expr->type == EXPR_ACCESS);
  expression_visitor(expr->data.access_expr.obj);
  expression_visitor(expr->data.access_expr.prop);
  if (expr->data.access_expr.prop->type != EXPR_CALL) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "GET");
  }
}

void expression_access_visitor_for_call(struct syntax_expr *expr) {
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "COPY");
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_S %s",
                   expr->data.call_expr.func_name);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "GET");
}

// LIT_INT, LIT_FLOAT, LIT_STR, LIT_BOOL
void expression_literal_visitor(struct syntax_expr *expr) {
  assert(expr->type == EXPR_LITERAL);
  switch (expr->data.literal_expr.kind) {
  case LIT_INT:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_D %d",
                     expr->data.literal_expr.int_val);
    break;
  case LIT_FLOAT:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_F %f",
                     expr->data.literal_expr.float_val);
    break;
  case LIT_STR:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_S %s",
                     expr->data.literal_expr.str_val);
    break;
  case LIT_BOOL:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_B %d",
                     expr->data.literal_expr.bool_val);
    break;
  case LIT_NULL:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_N");
    break;
  default:
    assert(false);
  }
}

void expression_binary_visitor(struct syntax_expr *expr) {
  assert(expr->type == EXPR_BINARY);
  expression_visitor(expr->data.binary_expr.left);
  if (expr->data.binary_expr.left->type == EXPR_ACCESS &&
      expr->data.binary_expr.op == TOKEN_ASSIGN) {
    INSTRUCTION_REMOVE_END(CURRENT_FUNCTION_NAME);
  }
  expression_visitor(expr->data.binary_expr.right);
  switch (expr->data.binary_expr.op) {
  case TOKEN_ADD:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "ADD");
    break;
  case TOKEN_MINUS:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "MINUS");
    break;
  case TOKEN_MULTI:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "MULTI");
    break;
  case TOKEN_DIV:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "DIV");
    break;
  case TOKEN_EQUAL:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "E");
    break;
  case TOKEN_NOT_EQUAL:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "NE");
    break;
  case TOKEN_GREATER:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "G");
    break;
  case TOKEN_GREATER_EQUAL:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "GE");
    break;
  case TOKEN_LESS:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "L");
    break;
  case TOKEN_LESS_EQUAL:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LE");
    break;
  case TOKEN_ASSIGN: {
    if (expr->data.binary_expr.left->type == EXPR_ACCESS) {
      INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "SET");
    } else {
      INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "ASSIGN");
      if (expr->data.binary_expr.left->data.identifier_expr.is_from_top) {
        INSTRUCTION_SAVE(
            CURRENT_FUNCTION_NAME, "STORE %d",
            expr->data.binary_expr.left->data.identifier_expr.var_index);
      } else {
        if (expr->data.binary_expr.left->data.identifier_expr.from_params) {
          INSTRUCTION_SAVE(
              CURRENT_FUNCTION_NAME, "STORE #-%d",
              expr->data.binary_expr.left->data.identifier_expr.offset);
        } else {
          INSTRUCTION_SAVE(
              CURRENT_FUNCTION_NAME, "STORE #%d",
              expr->data.binary_expr.left->data.identifier_expr.var_index + 1);
        }
      }
    }
  } break;
  case TOKEN_AND:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "AND");
    break;
  case TOKEN_OR:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "OR");
    break;
  default:
    assert(false);
  }
}

void expression_unary_visitor(struct syntax_expr *expr) {
  assert(expr->type == EXPR_UNARY);
  expression_visitor(expr->data.binary_expr.right);
  switch (expr->data.binary_expr.op) {
  case TOKEN_MINUS:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "NEGATE");
    break;
  case TOKEN_NOT:
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "NOT");
    break;
  default:
    assert(false);
  }
}

/*
a.name(1, 2)

    [0]...
    [1] a.name <----|
    [2] 2           |
    [3] 1           |__ call -2
    [4]
    [5]
    [6]
*/
void expression_call_visitor(struct syntax_expr *expr) {
  assert(expr->type == EXPR_CALL);
  if (expr->data.call_expr.source_in_stack) {
    expression_access_visitor_for_call(expr);
  }

  if (expr->data.call_expr.is_from_params) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD #-%d",
                     expr->data.call_expr.offset);
  } else if (expr->data.call_expr.var_index != -1 &&
             !expr->data.call_expr.source_in_stack) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD #%d",
                     expr->data.call_expr.var_index + 1);
  }
  for (int i = expr->data.call_expr.args->count - 1; i >= 0; i--) {
    expression_visitor(
        expr->data.call_expr.args->get(expr->data.call_expr.args, i));
  }
  if (expr->data.call_expr.source_in_stack) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL -%d",
                     expr->data.call_expr.args->count);
  } else if (expr->data.call_expr.is_from_params ||
             expr->data.call_expr.var_index != -1) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL -%d",
                     expr->data.call_expr.args->count);
  } else {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL %s",
                     expr->data.call_expr.func_name);
  }
  if (expr->data.call_expr.source_in_stack) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "FREE %d",
                     expr->data.call_expr.args->count + 2); // function and this
  } else if (expr->data.call_expr.is_from_params ||
             expr->data.call_expr.var_index != -1) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "FREE %d",
                     expr->data.call_expr.args->count + 1); // function
  } else {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "FREE %d",
                     expr->data.call_expr.args->count);
  }
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD [ei]");
}

void expression_array_visitor(struct syntax_expr *expr) {
  assert(expr->type == EXPR_ARRAY);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_S __data__");
  struct Vec *elements = expr->data.array_expr.elements;
  for (int i = 0; i < elements->count; i++) {
    expression_visitor(elements->get(elements, i));
  }
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_D %d", elements->count);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL native_new_array");
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "FREE %d", elements->count + 1);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD [ei]");
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_S count");
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_D %d", elements->count);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_D 2");
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL native_new_object");
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "FREE %d", 2 * 2 + 1);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD [ei]");
}
void expression_object_visitor(struct syntax_expr *expr) {
  assert(expr->type == EXPR_OBJECT);
  struct Vec *pairs = expr->data.object_expr.pairs;
  for (int i = 0; i < pairs->count; i++) {
    struct syntax_kv_pair *kv = pairs->get(pairs, i);
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_S %s", kv->key);
    expression_visitor(kv->value);
  }
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_D %d", pairs->count);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL native_new_object");
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "FREE %d", pairs->count * 2 + 1);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD [ei]");
}

typedef enum {
  VAL_INT,
  VAL_FLOAT,
  VAL_BOOL,
  VAL_CHAR,
  VAL_REF,
  VAL_OFFSET,
  VAL_REGISTER,
  VAL_NULL,
} ValueType;

typedef enum {
  REF_VAL_OBJ,
  REF_VAL_ARR,
  REF_VAL_FUNC,
  REF_VAL_STR,
  REF_VAL_STR_RAW
} RefValueType;

typedef struct {
  ValueType type;
  union {
    int i_val;
    float f_val;
    bool b_val;
    void *ptr;
    char c_val;
  } data;
} ZValue;

typedef struct {
  int length;
  int capacity;
  ZValue **elements;
} ZArray;

typedef struct {
  const char *key;
  ZValue *value;
} ZPair;

typedef struct zero_object {
  int count;
  ZPair *entries;
  struct zero_object *prototype;
} ZObject;

typedef struct {
  char *raw;
} ZRawString;

// ZString defination area
#define ZString ZObject
#define ZSTRING_PROP_LEN 5

typedef struct {
  // INSTRUCTION *instructions;
  struct Vec *instructions;
  bool is_builtin;
  const char *name;
  int params_count;
} ZFunction;

typedef struct {
  RefValueType type;
  uint32_t ref_count;
  union {
    ZObject *obj;
    ZArray *arr;
    ZFunction *func;
    ZRawString *rstring;
  } data;
} ZRefValue;

typedef struct zero_memory_allocator_params {
  struct {
    size_t str_length;
  } str;

  struct {
    size_t elem_count;
  } arr;

  struct {
    size_t kv_count;
  } obj;
  struct {
    size_t inst_count;
    bool new_build;
  } func;

  struct {
    size_t register_name_size;
  } reg;

  struct {
    bool is_shell;
  } ref;
} AllocatorParams;

ZValue *allocator_data(MemoryManager *manager, int type, int ref_type,
                       AllocatorParams *params);
ZFunction *allocator_func_data(MemoryManager *manager, AllocatorParams *params);
#define PROTOTYPE_PROP_COUNT 1

ZObject *allocator_object_prototype(MemoryManager *manager, ZValue *t,
                                    AllocatorParams *params) {
  ZObject *prototype = (ZObject *)malloc(sizeof(ZObject));
  // init prototype count
  {
    prototype->count = PROTOTYPE_PROP_COUNT;
  }
  // init prototype item
  {
    prototype->entries = (ZPair *)malloc(sizeof(ZPair) * prototype->count);
    prototype->entries[0].key = "__hash__";
    ZValue *v = allocator_data(manager, VAL_REF, REF_VAL_FUNC, NULL);
    prototype->entries[0].value = v;
    ZRefValue *ref = (ZRefValue *)v->data.ptr;
    ref->data.func->is_builtin = true;
    ref->data.func->name = "__hash__";
  }

  prototype->prototype = NULL;
  memory_allocator(manager, sizeof(ZObject) + sizeof(ZPair) * prototype->count);
  return prototype;
}

ZObject *allocator_object_data(MemoryManager *manager,
                               AllocatorParams *params) {
  ZObject *obj = (ZObject *)malloc(sizeof(ZObject));
  obj->count = params->obj.kv_count;
  if (obj->count != 0) {
    obj->entries = (ZPair *)malloc(sizeof(ZPair) * obj->count);
  } else {
    obj->entries = NULL;
  }
  obj->prototype = allocator_object_prototype(manager, obj, params);

  memory_allocator(manager, sizeof(ZObject) + sizeof(ZPair) * obj->count);
  return obj;
}
ZArray *allocator_array_data(MemoryManager *manager, AllocatorParams *params) {
  ZArray *arr = (ZArray *)malloc(sizeof(ZArray));
  arr->length = params->arr.elem_count;
  arr->capacity = arr->length * 2 == 0 ? 2 : arr->length * 2;
  if (arr->capacity != 0) {
    arr->elements = (ZValue **)malloc(sizeof(ZValue *) * arr->capacity);
  } else {
    arr->elements = NULL;
  }

  memory_allocator(manager, sizeof(ZArray) + sizeof(ZValue *) * arr->capacity);
  return arr;
}

ZString *allocator_string_data(MemoryManager *manager,
                               AllocatorParams *params) {
  params->obj.kv_count = ZSTRING_PROP_LEN;
  ZString *s = allocator_object_data(manager, params);

  s->entries[0].key = "value";
  ZValue *vz = allocator_data(manager, VAL_REF, REF_VAL_STR_RAW, params);
  s->entries[0].value = vz;
  s->entries[1].key = "len";
  ZValue *lv = allocator_data(manager, VAL_REF, REF_VAL_FUNC, NULL);
  ((ZRefValue *)lv->data.ptr)->data.func->is_builtin = true;
  ((ZRefValue *)lv->data.ptr)->data.func->name = "native_string_len";
  s->entries[1].value = lv;

  s->entries[2].key = "substr";
  ZValue *sv = allocator_data(manager, VAL_REF, REF_VAL_FUNC, NULL);
  ((ZRefValue *)sv->data.ptr)->data.func->is_builtin = true;
  ((ZRefValue *)sv->data.ptr)->data.func->name = "native_string_substr";
  s->entries[2].value = sv;

  s->entries[3].key = "equal";
  ZValue *ev = allocator_data(manager, VAL_REF, REF_VAL_FUNC, NULL);
  ((ZRefValue *)ev->data.ptr)->data.func->is_builtin = true;
  ((ZRefValue *)ev->data.ptr)->data.func->name = "native_string_equal";
  s->entries[3].value = ev;

  s->entries[4].key = "concat";
  ZValue *cv = allocator_data(manager, VAL_REF, REF_VAL_FUNC, NULL);
  ((ZRefValue *)cv->data.ptr)->data.func->is_builtin = true;
  ((ZRefValue *)cv->data.ptr)->data.func->name = "native_string_concat";
  s->entries[4].value = cv;

  return s;
}

ZFunction *allocator_func_data(MemoryManager *manager,
                               AllocatorParams *params) {
  if (params != NULL && !params->func.new_build) {
    return NULL;
  }
  ZFunction *func = (ZFunction *)malloc(sizeof(ZFunction));
  func->instructions = new_vec();
  func->is_builtin = false;
  func->name = NULL;
  memory_allocator(manager, sizeof(ZFunction));
  return func;
}

ZFunction *allocator_raw_string_data(MemoryManager *manager,
                                     AllocatorParams *params) {
  char *s_value = NULL;
  if (params->str.str_length != 0) {
    s_value = (char *)malloc(sizeof(char) * params->str.str_length);
    *(s_value + params->str.str_length - 1) = '\0';
  }
  ZRawString *v = (ZRawString *)malloc(sizeof(ZRawString));
  v->raw = s_value;
  memory_allocator(manager,
                   sizeof(ZRawString) + sizeof(char) * params->str.str_length);
  return v;
}

ZRefValue *allocator_ref_data(MemoryManager *manager, int ref_type,
                              AllocatorParams *params) {
  ZRefValue *ref = (ZRefValue *)malloc(sizeof(ZRefValue));
  ref->type = ref_type;
  switch (ref_type) {
  case REF_VAL_STR: {
    ref->data.obj = allocator_string_data(manager, params);
  } break;
  case REF_VAL_ARR: {
    ref->data.arr = allocator_array_data(manager, params);
  } break;
  case REF_VAL_OBJ: {
    ref->data.obj = allocator_object_data(manager, params);
  } break;
  case REF_VAL_FUNC: {
    ref->data.func = allocator_func_data(manager, params);
  } break;
  case REF_VAL_STR_RAW: {
    ref->data.rstring = allocator_raw_string_data(manager, params);
  } break;
  default:
    free(ref);
    assert(false);
  }
  memory_allocator(manager, sizeof(ZRefValue));
  ref->ref_count = 1;
  return ref;
}

ZValue *allocator_data(MemoryManager *manager, int type, int ref_type,
                       AllocatorParams *params) {
  switch (type) {
  case VAL_REF: {
    ZValue *v = (ZValue *)malloc(sizeof(ZValue));
    v->type = VAL_REF;
    memory_allocator(manager, sizeof(ZValue));
    if (params == NULL || !params->ref.is_shell) {
      v->data.ptr = allocator_ref_data(manager, ref_type, params);
    } else {
      v->data.ptr = NULL;
    }
    return v;
  } break;
  case VAL_BOOL:
  case VAL_FLOAT:
  case VAL_INT:
  case VAL_CHAR:
  case VAL_NULL:
  case VAL_OFFSET: {
    ZValue *v = (ZValue *)malloc(sizeof(ZValue));
    v->type = type;
    memory_allocator(manager, sizeof(ZValue));
    return v;
  }
  case VAL_REGISTER: {
    ZValue *v = (ZValue *)malloc(sizeof(ZValue));
    v->type = VAL_REGISTER;
    v->data.ptr = (char *)malloc(sizeof(char) * params->reg.register_name_size);
    memory_allocator(manager,
                     sizeof(ZValue) +
                         sizeof(char) * params->reg.register_name_size);
    return v;
  }
  default:
    assert(false);
  }
}
void deallocator_obj_data(MemoryManager *manager, ZObject *obj);
void deallocator_data(MemoryManager *manager, ZValue *value);

void deallocator_sting_data(MemoryManager *manager, ZString *str) {
  deallocator_obj_data(manager, str);
}

void deallocator_raw_string_data(MemoryManager *manager, ZRawString *str) {
  if (str == NULL) {
    return;
  }

  int len = strlen(str->raw);
  free(str->raw);
  free(str);
  memory_deallocator(manager, sizeof(ZRawString) + sizeof(char) * (len + 1));
}

void deallocator_func_data(MemoryManager *manager, ZFunction *func) {
  // ZFunction is GOD! can't be free~
  return;
}

void deallocator_arr_data(MemoryManager *manager, ZArray *arr) {
  if (arr == NULL) {
    return;
  }
  if (arr->length != 0) {
    for (int i = 0; i < arr->length; i++) {
      ZValue *e = arr->elements[i];
      deallocator_data(manager, e);
    }
  }
  memory_deallocator(manager, (sizeof(ZValue *)) * (arr->capacity));
  free(arr->elements);
  free(arr);
  memory_deallocator(manager, sizeof(ZArray));
}

void deallocator_obj_data(MemoryManager *manager, ZObject *obj) {
  if (obj == NULL) {
    return;
  }
  if (obj->count != 0) {
    for (int i = 0; i < obj->count; i++) {
      ZValue *e = obj->entries[i].value;
      deallocator_data(manager, e);
    }
    memory_deallocator(manager, sizeof(ZPair));
    free(obj->entries);
  }
  deallocator_data(manager, obj->prototype);
  free(obj);
  memory_deallocator(manager, sizeof(ZObject));
}

void deallocator_ref_data(MemoryManager *manager, ZRefValue *ref) {
  if (ref == NULL) {
    return;
  }
  if (ref->ref_count >= 2) {
    ref->ref_count -= 1;
    return;
  }
  // printf("DEBUG: free %ld\n", ref);
  switch (ref->type) {
  case REF_VAL_ARR: {
    deallocator_arr_data(manager, ref->data.arr);
  } break;
  case REF_VAL_FUNC: {
    deallocator_func_data(manager, ref->data.func);
  } break;
  case REF_VAL_OBJ: {
    deallocator_obj_data(manager, ref->data.obj);
  } break;
  case REF_VAL_STR: {
    deallocator_sting_data(manager, ref->data.obj);
  } break;
  case REF_VAL_STR_RAW: {
    deallocator_raw_string_data(manager, ref->data.rstring);
  } break;
  default:
    assert(false);
  }
  free(ref);
  memory_deallocator(manager, sizeof(ZRefValue));
}

void deallocator_data(MemoryManager *manager, ZValue *value) {
  if (value == NULL) {
    return;
  }
  switch (value->type) {
  case VAL_REF: {
    ZRefValue *ref = (ZRefValue *)value->data.ptr;
    deallocator_ref_data(manager, ref);
    free(value);
    memory_deallocator(manager, sizeof(ZValue));
  } break;
  case VAL_INT:
  case VAL_BOOL:
  case VAL_FLOAT:
  case VAL_NULL:
  case VAL_REGISTER:
  case VAL_CHAR:
  case VAL_OFFSET: {
    free(value);
    memory_deallocator(manager, sizeof(ZValue));
  } break;
  default:
    assert(false);
  }
}

typedef enum {
  I_PUSH,
  I_MULTI,
  I_DIV,
  I_ADD,
  I_MINUS,
  I_LOAD,
  I_STORE,
  I_CALL,
  I_G,
  I_GE,
  I_L,
  I_LE,
  I_E,
  I_NE,
  I_AND,
  I_OR,
  I_NOT,
  I_NEGATE,
  I_ASSIGN,
  I_FREE,
  I_JUMP,
  I_JF, // if stack top is true, jump, else do nothing
  I_GET,
  I_SET,
  I_CHECK,
  I_COPY
} INSTRUCTION_CODE;

struct zero_context {
  struct zero_context *parent;
  int pc; // current pc
  int bp;
};

#define Context struct zero_context

struct zero_vm {
  struct Vec *globals;
  ZValue *entry;
  Context *root_context;
  Context *cur_context;
  Map *ready_link;
  int sp; // stack top
  Map *registers;
  Map *symbols; // string -> ZValue
  struct Vec *cvalues;
  MemoryManager *mm;
  void *stacks[1024];
};

#define VM struct zero_vm

ZValue *copy(VM *vm, ZValue *src);

typedef struct {
  INSTRUCTION_CODE code;
  ZValue *v;
} INSTRUCTION;

INSTRUCTION *new_inst(INSTRUCTION_CODE code, ZValue *v) {
  INSTRUCTION *inst = (INSTRUCTION *)malloc(sizeof(INSTRUCTION));
  inst->code = code;
  inst->v = v;
  return inst;
}

typedef struct {
  ZFunction *func;
  Context *ctx;
} Runnable;

Context *new_context() {
  Context *ctx = (Context *)malloc(sizeof(Context));
  ctx->parent = NULL;
  ctx->bp = 0;
  ctx->pc = 0;
  return ctx;
}

void free_runnable(Runnable *runnable) {
  free(runnable->ctx);
  free(runnable);
}

const Map *actions = NULL;

typedef INSTRUCTION *(*action_for_inst)(VM *vm, const char *value);

INSTRUCTION *NEW_STORE_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_LOAD_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_PUSH_D_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_PUSH_F_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_PUSH_B_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_PUSH_S_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_PUSH_N_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_CALL_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_ADD_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_MINUS_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_MULTI_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_DIV_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_G_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_GE_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_L_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_LE_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_NEGATE_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_NOT_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_EQUAL_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_NOT_EQUAL_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_ASSIGN_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_FREE_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_JUMP_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_JF_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_GET_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_SET_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_CHECK_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_COPY_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_AND_INSTRUCTION(VM *vm, const char *value);
INSTRUCTION *NEW_OR_INSTRUCTION(VM *vm, const char *value);
void init_actions() {
  actions = (Map *)malloc(sizeof(Map));
  map_insert(actions, "STORE", NEW_STORE_INSTRUCTION);
  map_insert(actions, "LOAD", NEW_LOAD_INSTRUCTION);
  map_insert(actions, "PUSH_D", NEW_PUSH_D_INSTRUCTION);
  map_insert(actions, "PUSH_F", NEW_PUSH_F_INSTRUCTION);
  map_insert(actions, "PUSH_B", NEW_PUSH_B_INSTRUCTION);
  map_insert(actions, "PUSH_S", NEW_PUSH_S_INSTRUCTION);
  map_insert(actions, "PUSH_N", NEW_PUSH_N_INSTRUCTION);
  map_insert(actions, "CALL", NEW_CALL_INSTRUCTION);
  map_insert(actions, "ADD", NEW_ADD_INSTRUCTION);
  map_insert(actions, "MINUS", NEW_MINUS_INSTRUCTION);
  map_insert(actions, "MULTI", NEW_MULTI_INSTRUCTION);
  map_insert(actions, "DIV", NEW_DIV_INSTRUCTION);
  map_insert(actions, "G", NEW_G_INSTRUCTION);
  map_insert(actions, "GE", NEW_GE_INSTRUCTION);
  map_insert(actions, "L", NEW_L_INSTRUCTION);
  map_insert(actions, "LE", NEW_LE_INSTRUCTION);
  map_insert(actions, "NOT", NEW_NOT_INSTRUCTION);
  map_insert(actions, "NEGATE", NEW_NEGATE_INSTRUCTION);
  map_insert(actions, "E", NEW_EQUAL_INSTRUCTION);
  map_insert(actions, "NE", NEW_NOT_EQUAL_INSTRUCTION);
  map_insert(actions, "ASSIGN", NEW_ASSIGN_INSTRUCTION);
  map_insert(actions, "FREE", NEW_FREE_INSTRUCTION);
  map_insert(actions, "JUMP", NEW_JUMP_INSTRUCTION);
  map_insert(actions, "JF", NEW_JF_INSTRUCTION);
  map_insert(actions, "GET", NEW_GET_INSTRUCTION);
  map_insert(actions, "SET", NEW_SET_INSTRUCTION);
  map_insert(actions, "CHECK", NEW_CHECK_INSTRUCTION);
  map_insert(actions, "COPY", NEW_COPY_INSTRUCTION);
  map_insert(actions, "AND", NEW_AND_INSTRUCTION);
  map_insert(actions, "OR", NEW_OR_INSTRUCTION);
}

ZValue *build_builtin_fuction(VM *vm, const char *name) {
  ZValue *f = allocator_data(vm->mm, VAL_REF, REF_VAL_FUNC, NULL);
  ((ZRefValue *)f->data.ptr)->data.func->name = name;
  ((ZRefValue *)(f->data.ptr))->data.func->is_builtin = true;
  return f;
}

void init_builtin(VM *vm) {
  map_insert(vm->symbols, "native_println",
             build_builtin_fuction(vm, "native_println"));
  map_insert(vm->symbols, "native_new_array",
             build_builtin_fuction(vm, "native_new_array"));
  map_insert(vm->symbols, "native_new_object",
             build_builtin_fuction(vm, "native_new_object"));
  map_insert(vm->symbols, "native_print",
             build_builtin_fuction(vm, "native_print"));

  map_insert(vm->symbols, "native_open",
             build_builtin_fuction(vm, "native_open"));

  map_insert(vm->symbols, "native_read",
             build_builtin_fuction(vm, "native_read"));

  map_insert(vm->symbols, "native_close",
             build_builtin_fuction(vm, "native_close"));

  map_insert(vm->symbols, "native_string_len",
             build_builtin_fuction(vm, "native_string_len"));

  map_insert(vm->symbols, "native_string_substr",
             build_builtin_fuction(vm, "native_string_substr"));

  map_insert(vm->symbols, "native_string_equal",
             build_builtin_fuction(vm, "native_string_equal"));
  map_insert(vm->symbols, "native_string_concat",
             build_builtin_fuction(vm, "native_string_concat"));

  map_insert(vm->symbols, "native_vector_insert",
             build_builtin_fuction(vm, "native_vector_insert"));
  map_insert(vm->symbols, "native_vector_get",
             build_builtin_fuction(vm, "native_vector_get"));

  map_insert(vm->symbols, "native_panic",
             build_builtin_fuction(vm, "native_panic"));
}

const Map *GET_ACTIONS() {
  if (actions == NULL) {
    init_actions();
  }
  return actions;
}

void list_inst_store(MapPair *pair, void *data) {
  VM *vm = (VM *)data;
  ZValue *f_v = allocator_data(vm->mm, VAL_REF, REF_VAL_FUNC, NULL);
  ZFunction *f = ((ZRefValue *)(f_v->data.ptr))->data.func;
  f->name = pair->key;
  struct str_store *store = (struct str_store *)pair->value;
  for (int i = 0; i < store->count; i++) {
    char *inst_str = store->get(store, i)->str;
    int len = strcspn(inst_str, " ");
    inst_str[len] = '\0';
    INSTRUCTION *inst =
        ((action_for_inst)(map_get(GET_ACTIONS(), inst_str)->value))(
            vm, inst_str + len + 1);
    inst_str[len] = ' ';
    f->instructions->push(f->instructions, inst);
  }

  if (strcmp(pair->key, "__init__") == 0) {
    vm->globals->push(vm->globals, f_v);
  } else {
    if (strcmp(pair->key, "main") == 0) {
      vm->entry = f_v;
    }
    map_insert(vm->symbols, pair->key, f_v);
  }
}

void vm_compile_stage_kernel(VM *vm, struct instruction_store *store) {
  if (store == NULL) {
    return;
  }
  for (int i = 0; i < store->childs->count; i++) {
    vm_compile_stage_kernel(vm, store->childs->get(store->childs, i));
  }
  MapPair pair = (MapPair){.key = "__init__", .value = store->static_func};
  list_inst_store(&pair, vm);
  map_foreach(store->map, list_inst_store, vm);
}

void vm_compile_stage(VM *vm) {
  vm_compile_stage_kernel(vm, root_instruction_store);
}

void vm_link_progress(MapPair *pair, void *data) {
  VM *vm = (VM *)data;
  struct Vec *funcs = (struct Vec *)pair->value;
  MapPair *r = map_get(vm->symbols, pair->key);
  if (r == NULL) {
    printf("linker error, %s not found.\n", pair->key);
    assert(false);
    return;
  }
  for (int i = 0; i < funcs->count; i++) {
    INSTRUCTION *inst = (INSTRUCTION *)funcs->get(funcs, i);
    ZValue *zv = (ZValue *)r->value;
    assert(zv->type == VAL_REF);
    ZRefValue *f = (ZRefValue *)zv->data.ptr;
    assert(f->type == REF_VAL_FUNC);
    ((ZRefValue *)(inst->v->data.ptr))->data.func = f->data.func;
  }
}

void vm_link_stage(VM *vm) {
  map_foreach(vm->ready_link, vm_link_progress, vm);
}

void VM_Init(VM *vm) {
  init_actions();
  init_builtin(vm);
  vm_compile_stage(vm);
  vm_link_stage(vm);
}

void PUSH_INST_RUN(VM *vm, ZValue *value) {
  vm->stacks[++vm->sp] = copy(vm, value);
}

void store_inst_run_helper(VM *vm, ZValue *v, int target_position) {
  // is new variable store,
  if (vm->sp < target_position) {
    vm->sp = target_position;
    if (v->type == VAL_REF) {
      ZValue *target = allocator_data(vm->mm, VAL_REF, 0,
                                      &(AllocatorParams){.ref.is_shell = true});
      target->data.ptr = v->data.ptr;
      ((ZRefValue *)target->data.ptr)->ref_count += 1;
      vm->stacks[target_position] = target;
    } else {
      ZValue *new_v = allocator_data(vm->mm, v->type, 0, NULL);
      new_v->data = v->data;
      vm->stacks[target_position] = new_v;
    }
  } else {
    ZValue *target = vm->stacks[target_position];
    if (target_position != vm->sp + 1) {
      deallocator_data(vm->mm, target);
    }
    if (v->type == VAL_REF) {
      target = allocator_data(vm->mm, VAL_REF, 0,
                              &(AllocatorParams){.ref.is_shell = true});

      target->data.ptr = v->data.ptr;
      ((ZRefValue *)target->data.ptr)->ref_count += 1;
      vm->stacks[target_position] = target;
    } else {
      ZValue *new_v = allocator_data(vm->mm, v->type, 0, NULL);
      new_v->data = v->data;
      vm->stacks[target_position] = new_v;
    }
  }
  deallocator_data(vm->mm, v);
}

void STORE_INST_RUN(VM *vm, ZValue *value) {
  ZValue *v = vm->stacks[vm->sp--];
  if (value->type == VAL_REGISTER) {
    ZValue *target = NULL;
    if (v->type == VAL_REF) {
      target = allocator_data(vm->mm, VAL_REF, 0,
                              &(AllocatorParams){.ref.is_shell = true});

      target->data.ptr = v->data.ptr;
      ((ZRefValue *)target->data.ptr)->ref_count += 1;
    } else {
      target = allocator_data(vm->mm, v->type, 0, NULL);
      target->data = v->data;
    }
    deallocator_data(vm->mm, v);
    map_insert(vm->registers, (const char *)(value->data.ptr), target);
  } else if (value->type == VAL_OFFSET) {
    /*
    When loading a reference-type data, the consumer should directly resolve or
    access the actual value being referenced, rather than the reference object
    itself.
    */
    int target_position = value->data.i_val + vm->cur_context->bp;
    store_inst_run_helper(vm, v, target_position);
  } else {
    store_inst_run_helper(vm, v, value->data.i_val);
  }
}

void LOAD_INST_RUN(VM *vm, ZValue *value) {
  /*
    VAL_OFFSET: load value by bp and offset;
    target_index = bp + offset;
  */
  if (value->type == VAL_OFFSET) {
    int offset = value->data.i_val;
    ZValue *src = vm->stacks[vm->cur_context->bp + offset];
    if (src->type == VAL_REF) {
      ZValue *target = allocator_data(vm->mm, VAL_REF, 0,
                                      &(AllocatorParams){.ref.is_shell = true});
      target->data.ptr = src->data.ptr;
      ((ZRefValue *)target->data.ptr)->ref_count += 1;
      vm->stacks[++vm->sp] = target;
    } else {
      ZValue *target = allocator_data(vm->mm, src->type, 0, NULL);
      target->data = src->data;
      vm->stacks[++vm->sp] = target;
    }
  } else if (value->type == VAL_REGISTER) {
    MapPair *rv = map_get(vm->registers, (const char *)(value->data.ptr));
    if (rv->value != NULL) {
      ZValue *src = rv->value;
      if (src->type == VAL_REF) {
        ZValue *target = allocator_data(
            vm->mm, VAL_REF, 0, &(AllocatorParams){.ref.is_shell = true});
        target->data.ptr = src->data.ptr;
        ((ZRefValue *)target->data.ptr)->ref_count += 1;
        vm->stacks[++vm->sp] = target;
      } else {
        ZValue *target = allocator_data(vm->mm, src->type, 0, NULL);
        target->data = src->data;
        vm->stacks[++vm->sp] = target;
      }
      deallocator_data(vm->mm, rv->value);
      rv->value = NULL;
    } else {
      ZValue *v = allocator_data(vm->mm, VAL_NULL, 0, NULL);
      v->type = VAL_NULL;
      vm->stacks[++vm->sp] = v;
    }
  } else if (value->type == VAL_INT) {
    /*
      VAL_INT
      load value from base offset,
      target_index = offset;
    */
    int offset = value->data.i_val;
    ZValue *src = vm->stacks[offset];
    if (src->type == VAL_REF) {
      ZValue *target = allocator_data(vm->mm, VAL_REF, 0,
                                      &(AllocatorParams){.ref.is_shell = true});
      target->data.ptr = src->data.ptr;
      ((ZRefValue *)target->data.ptr)->ref_count += 1;
      vm->stacks[++vm->sp] = target;
    } else {
      ZValue *target = allocator_data(vm->mm, src->type, 0, NULL);
      target->data = src->data;
      vm->stacks[++vm->sp] = target;
    }
  } else if (value->type == VAL_REF) {
    /*
      LOAD .func_name
    */
    assert(((ZRefValue *)value->data.ptr)->type == REF_VAL_FUNC);
    ZValue *target = allocator_data(vm->mm, VAL_REF, 0,
                                    &(AllocatorParams){.ref.is_shell = true});
    target->data.ptr = value->data.ptr;
    ((ZRefValue *)target->data.ptr)->ref_count += 1;
    vm->stacks[++vm->sp] = target;
  } else {
    assert(false);
  }
}

ZValue *inst_run_op(VM *vm, TOKEN_TYPE type, ZValue *left, ZValue *right) {
  if (left->type == VAL_REF) {
    left = (ZValue *)(left->data.ptr);
  }
  if (right->type == VAL_REF) {
    right = (ZValue *)(right->data.ptr);
  }
  assert(left->type == VAL_INT || left->type == VAL_FLOAT ||
         left->type == VAL_BOOL || left->type == VAL_CHAR);
  assert(right->type == VAL_INT || right->type == VAL_FLOAT ||
         right->type == VAL_BOOL || left->type == VAL_CHAR);
  bool is_int = false;
  if (left->type == VAL_INT || right->type == VAL_INT) {
    is_int = true;
  }

  float l = 0;
  float r = 0;
  if (left->type == VAL_INT) {
    l = left->data.i_val;
  } else if (left->type == VAL_FLOAT) {
    l = left->data.f_val;
  } else if (left->type == VAL_BOOL) {
    l = left->data.b_val ? 1 : 0;
  } else if (left->type == VAL_CHAR) {
    l = left->data.c_val;
  }

  if (right->type == VAL_INT) {
    r = right->data.i_val;
  } else if (right->type == VAL_FLOAT) {
    r = right->data.f_val;
  } else if (right->type == VAL_BOOL) {
    r = right->data.b_val ? 1 : 0;
  } else if (right->type == VAL_CHAR) {
    r = right->data.c_val;
  }
  float s = 0;
  switch (type) {
  case TOKEN_ADD:
    s = l + r;
    break;
  case TOKEN_MINUS:
    s = l - r;
    break;
  case TOKEN_MULTI:
    s = l * r;
    break;
  case TOKEN_DIV:
    s = l / r;
    break;
  default:
    break;
  }
  ZValue *result = NULL;
  if (is_int) {
    result = allocator_data(vm->mm, VAL_INT, 0, NULL);
    result->data.i_val = (int)s;
  } else {
    result = allocator_data(vm->mm, VAL_FLOAT, 0, NULL);
    result->data.f_val = s;
  }
  deallocator_data(vm->mm, vm->stacks[vm->sp]);
  deallocator_data(vm->mm, vm->stacks[vm->sp - 1]);
  return result;
}

void ADD_INST_RUN(VM *vm, ZValue *value) {
  ZValue *result =
      inst_run_op(vm, TOKEN_ADD, vm->stacks[vm->sp - 1], vm->stacks[vm->sp]);
  vm->stacks[--(vm->sp)] = result;
}

void MINUS_INST_RUN(VM *vm, ZValue *value) {
  ZValue *result =
      inst_run_op(vm, TOKEN_MINUS, vm->stacks[vm->sp - 1], vm->stacks[vm->sp]);
  vm->stacks[--(vm->sp)] = result;
}

void MULTI_INST_RUN(VM *vm, ZValue *value) {
  ZValue *result =
      inst_run_op(vm, TOKEN_MULTI, vm->stacks[vm->sp - 1], vm->stacks[vm->sp]);
  vm->stacks[--(vm->sp)] = result;
}

float get_number_value_from_zvalue(VM *vm, ZValue *v) {
  if (v->type == VAL_INT) {
    return v->data.i_val;
  } else if (v->type == VAL_FLOAT) {
    return v->data.f_val;
  } else if (v->type == VAL_BOOL) {
    return v->data.b_val;
  } else if (v->type == VAL_REF) {
    return (int)v->data.ptr;
  } else if (v->type == VAL_NULL) {
    return 0;
  } else if (v->type == VAL_CHAR) {
    return v->data.c_val;
  } else {
    assert(false);
  }
}

void LOGIC_INST_RUN(VM *vm, ZValue *value, INSTRUCTION_CODE code) {
  ZValue *result = allocator_data(vm->mm, VAL_BOOL, 0, NULL);
  bool v = false;
  if (code >= I_G && code <= I_OR) {
    float left = get_number_value_from_zvalue(vm, vm->stacks[vm->sp - 1]);
    float right = get_number_value_from_zvalue(vm, vm->stacks[vm->sp]);
    switch (code) {
    case I_G:
      v = left > right;
      break;
    case I_GE:
      v = left >= right;
      break;
    case I_L:
      v = left < right;
      break;
    case I_LE:
      v = left <= right;
      break;
    case I_E:
      v = left == right;
      break;
    case I_NE:
      v = left != right;
      break;
    case I_AND:
      v = left && right;
      break;
    case I_OR:
      v = left || right;
      break;
    default:
      assert(false);
      break;
    }
    result->data.b_val = v;
    deallocator_data(vm->mm, vm->stacks[vm->sp]);
    deallocator_data(vm->mm, vm->stacks[vm->sp - 1]);
    vm->stacks[--vm->sp] = result;
  } else if (code == I_NOT) {
    float right = get_number_value_from_zvalue(vm, vm->stacks[vm->sp]);
    result->data.b_val = !right;
    deallocator_data(vm->mm, vm->stacks[vm->sp]);
    vm->stacks[vm->sp] = result;
  } else if (code == I_NEGATE) {
    ZValue *rv = vm->stacks[vm->sp];
    float v = -1 * get_number_value_from_zvalue(vm, rv);
    if (rv->type == VAL_REF) {
      rv = (ZValue *)rv->data.ptr;
    }
    ZValue *re = NULL;
    switch (rv->type) {
    case VAL_INT: {
      re = allocator_data(vm->mm, VAL_INT, 0, NULL);
      re->data.i_val = v;
    } break;
    case VAL_FLOAT: {
      re = allocator_data(vm->mm, VAL_FLOAT, 0, NULL);
      re->data.f_val = v;
    } break;
    case VAL_BOOL: {
      re = allocator_data(vm->mm, VAL_BOOL, 0, NULL);
      re->data.b_val = v == 0;
    } break;
    default:
      assert(false);
      break;
    }
    deallocator_data(vm->mm, vm->stacks[vm->sp]);
    vm->stacks[vm->sp] = re;
  }
}

void ASSIGN_INST_RUN(VM *vm, ZValue *value) {
  ZValue *source = vm->stacks[vm->sp--];
  ZValue *target = vm->stacks[vm->sp];
  if (target->type == VAL_REF) {
    deallocator_ref_data(vm->mm, target->data.ptr);
    target->data.ptr = NULL;
  } else {
    deallocator_data(vm->mm, target);
  }
  if (source->type == VAL_REF) {
    if (target->type != VAL_REF) {
      target = allocator_data(vm->mm, VAL_REF, 0,
                              &(AllocatorParams){.ref.is_shell = true});
    }
    target->data.ptr = source->data.ptr;
    ((ZRefValue *)target->data.ptr)->ref_count += 1;
    vm->stacks[vm->sp] = target;
  } else {
    ZValue *new_source = allocator_data(vm->mm, source->type, 0, NULL);
    new_source->data = source->data;
    vm->stacks[vm->sp] = new_source;
  }
  deallocator_data(vm->mm, source);
}

void DIV_INST_RUN(VM *vm, ZValue *value) {
  ZValue *result =
      inst_run_op(vm, TOKEN_DIV, vm->stacks[vm->sp - 1], vm->stacks[vm->sp]);
  vm->stacks[--(vm->sp)] = result;
}

void FREE_INST_RUN(VM *vm, ZValue *value) {
  assert(value->type == VAL_INT);
  int target_size = vm->sp - value->data.i_val;
  for (int i = vm->sp; i > target_size; i--) {
    deallocator_data(vm->mm, vm->stacks[i]);
  }
  vm->sp = target_size;
}

void print_string(VM *vm, ZValue *value);
void print_ref(VM *vm, ZValue *v);
ZValue *object_prop_get(VM *vm, ZObject *raw_obj, const char *key);

void print_data(VM *vm, ZValue *v) {
  switch (v->type) {
  case VAL_INT:
    printf("%d", v->data.i_val);
    break;
  case VAL_FLOAT:
    printf("%f", v->data.f_val);
    break;
  case VAL_BOOL: {
    if (v->data.b_val) {
      printf("true");
    } else {
      printf("false");
    }
  } break;
  case VAL_CHAR: {
    printf("%s", v->data.c_val);
  } break;
  case VAL_REF:
    print_ref(vm, v);
    break;
  case VAL_NULL:
    printf("null");
    break;
  default:
    break;
  }
}

void print_arr(VM *vm, ZValue *value) {
  ZRefValue *v = (ZRefValue *)value->data.ptr;
  ZArray *arr = v->data.arr;
  printf("[");
  for (int i = 0; i < arr->length; i++) {
    print_data(vm, *(arr->elements + i));
    if (i != arr->length - 1) {
      printf(", ");
    }
  }
  printf("]");
}

void CALL_INST_RUN(VM *vm, ZValue *value);

void print_string(VM *vm, ZValue *value) {
  ZRefValue *str_obj = (ZRefValue *)value->data.ptr;
  assert(str_obj->type == REF_VAL_STR);
  ZValue *str_raw_value = object_prop_get(vm, str_obj->data.obj, "value");
  if (str_raw_value == NULL) {
    return;
  }
  assert(str_raw_value->type == VAL_REF);
  print_ref(vm, str_raw_value);
}

void print_object(VM *vm, ZValue *value) {
  ZRefValue *v = (ZRefValue *)value->data.ptr;
  ZObject *obj = v->data.obj;
  ZValue *user_print_func = NULL;
  for (int i = 0; i < obj->count; i++) {
    if (strcmp("__str__", obj->entries[i].key) == 0) {
      user_print_func = obj->entries[i].value;
      break;
    }
  }
  if (user_print_func != NULL) {
    // push this pointer: LOAD xxx
    ZValue *this_ref = allocator_data(vm->mm, VAL_REF, 0,
                                      &(AllocatorParams){.ref.is_shell = true});
    this_ref->data.ptr = value->data.ptr;
    ((ZRefValue *)this_ref->data.ptr)->ref_count += 1;
    vm->stacks[++vm->sp] = this_ref;
    // push function object
    ZValue *func_ref = allocator_data(vm->mm, VAL_REF, 0,
                                      &(AllocatorParams){.ref.is_shell = true});
    func_ref->data.ptr = user_print_func->data.ptr;
    ((ZRefValue *)func_ref->data.ptr)->ref_count += 1;
    vm->stacks[++vm->sp] = func_ref;
    ZValue *call_index = allocator_data(vm->mm, VAL_INT, 0, NULL);
    call_index->data.i_val = 0;
    CALL_INST_RUN(vm, call_index);
    deallocator_data(vm->mm, this_ref);
    deallocator_data(vm->mm, func_ref);
    vm->sp -= 2;
  } else {
    printf("{");
    for (int i = 0; i < obj->count; i++) {
      printf("\"%s\":", obj->entries[i].key);
      ZValue *u = obj->entries[i].value;
      print_data(vm, u);
      if (i != obj->count - 1) {
        printf(", ");
      }
    }
    printf("}");
  }
}

void print_ref(VM *vm, ZValue *v) {
  assert(v->type == VAL_REF);
  ZRefValue *d = (ZRefValue *)(v->data.ptr);
  switch (d->type) {
  case REF_VAL_ARR:
    print_arr(vm, v);
    break;
  case REF_VAL_OBJ:
    print_object(vm, v);
    break;
  case REF_VAL_STR:
    print_string(vm, v);
    break;
  case REF_VAL_FUNC: {
    ZFunction *f = d->data.func;
    printf("[FUNCTION %s]", f->name);
  } break;
  case REF_VAL_STR_RAW: {
    printf("%s", d->data.rstring->raw);
  } break;
  default:
    assert(false);
    break;
  }
}

void native_print(VM *vm) {
  ZValue *v = vm->stacks[vm->sp];
  print_data(vm, v);
}

void native_println(VM *vm) {
  native_print(vm);
  printf("\n");
}

/*
1:v1
2:v2
3:2  <---- sp
*/

void native_new_array(VM *vm) {
  ZValue *count = vm->stacks[vm->sp];
  assert(count->type == VAL_INT);
  ZRefValue *data = allocator_ref_data(
      vm->mm, REF_VAL_ARR,
      &(AllocatorParams){.arr.elem_count = count->data.i_val});
  ZArray *arr = data->data.arr;
  for (int i = 0; i < arr->length; i++) {
    ZValue *src = vm->stacks[vm->sp - arr->length + i];
    arr->elements[i] = copy(vm, src);
  }
  ZValue *i = allocator_data(vm->mm, VAL_REF, 0,
                             &(AllocatorParams){.ref.is_shell = true});
  i->data.ptr = data;
  map_insert(vm->registers, "ei", i);
}

/*
1:k1
2:v1
3:k2
4:v2
5:2  <---- sp
*/

void native_new_object(VM *vm) {
  ZValue *count = vm->stacks[vm->sp];
  assert(count->type == VAL_INT);
  ZRefValue *data =
      allocator_ref_data(vm->mm, REF_VAL_OBJ,
                         &(AllocatorParams){.obj.kv_count = count->data.i_val});
  ZObject *obj = data->data.obj;
  for (int i = 0; i < obj->count; i++) {
    int stack_base = vm->sp - obj->count * 2;
    ZValue *key = vm->stacks[stack_base + i * 2];
    assert(key->type == VAL_REF);
    ZRefValue *key_ref = (ZRefValue *)key->data.ptr;
    assert(key_ref->type == REF_VAL_STR);
    ZValue *kv = object_prop_get(vm, key_ref->data.obj, "value");
    assert(kv != NULL);
    assert(kv->type = VAL_REF);
    ZRefValue *kv_ref = (ZRefValue *)kv->data.ptr;
    assert(kv_ref->type == REF_VAL_STR_RAW);

    obj->entries[i].key =
        (char *)malloc(sizeof(char) * (strlen(kv_ref->data.rstring->raw) + 1));
    memcpy(obj->entries[i].key, kv_ref->data.rstring->raw,
           strlen(kv_ref->data.rstring->raw) + 1);
    ZValue *value = vm->stacks[stack_base + i * 2 + 1];
    obj->entries[i].value = copy(vm, value);
  }
  ZValue *i = allocator_data(vm->mm, VAL_REF, 0,
                             &(AllocatorParams){.ref.is_shell = true});
  i->data.ptr = data;
  map_insert(vm->registers, "ei", i);
}

void zero_hash(VM *vm) {
  ZValue *this = vm->stacks[vm->sp - 1];
  assert(this->type == VAL_REF);
  ZRefValue *ref = (ZRefValue *)this->data.ptr;
  assert(ref->type == REF_VAL_OBJ);
  ZValue *v = allocator_data(vm->mm, VAL_INT, 0, NULL);
  v->data.i_val = (int)ref;
  map_insert(vm->registers, "ei", v);
}

void native_string_len(VM *vm) {
  ZValue *str = vm->stacks[vm->sp - 1];
  assert(str->type == VAL_REF);
  ZRefValue *ref = (ZRefValue *)str->data.ptr;
  assert(ref->type == REF_VAL_STR);
  ZValue *ret = allocator_data(vm->mm, VAL_INT, 0, NULL);
  ZValue *value = object_prop_get(vm, ref->data.obj, "value");

  ZRefValue *value_ref = (ZRefValue *)value->data.ptr;
  assert(value_ref->type == REF_VAL_STR_RAW);
  if (value != NULL) {
    ret->data.i_val = strlen(value_ref->data.rstring->raw);
  } else {
    ret->data.i_val = 0;
  }
  map_insert(vm->registers, "ei", ret);
}

void native_string_equal(VM *vm) {
  ZValue *ls = vm->stacks[vm->sp - 2];
  ZValue *rs = vm->stacks[vm->sp];
  assert(ls->type == VAL_REF);
  assert(rs->type == VAL_REF);
  ZRefValue *lref = (ZRefValue *)ls->data.ptr;
  ZRefValue *rref = (ZRefValue *)rs->data.ptr;
  assert(lref->type == REF_VAL_STR && rref->type == REF_VAL_STR);
  ZValue *lv = object_prop_get(vm, lref->data.obj, "value");
  ZValue *rv = object_prop_get(vm, rref->data.obj, "value");
  ZValue *result = allocator_data(vm->mm, VAL_BOOL, 0, NULL);
  ZRefValue *lv_ref = (ZRefValue *)lv->data.ptr;
  ZRefValue *rv_ref = (ZRefValue *)rv->data.ptr;
  assert(lv_ref->type == REF_VAL_STR_RAW && rv_ref->type == REF_VAL_STR_RAW);
  if (lv != NULL && rv != NULL) {
    int cmp = strcmp(lv_ref->data.rstring->raw, rv_ref->data.rstring->raw);
    result->data.b_val = cmp == 0;
  } else {
    result->data.b_val = false;
  }
  map_insert(vm->registers, "ei", result);
}

void native_string_concat(VM *vm) {
  ZValue *src = vm->stacks[vm->sp];
  ZValue *target = vm->stacks[vm->sp - 2];

  assert(src->type == VAL_REF);
  assert(target->type == VAL_REF);

  ZRefValue *tref = (ZRefValue *)target->data.ptr;
  assert(tref->type == REF_VAL_STR);

  ZRefValue *sref = (ZRefValue *)src->data.ptr;
  assert(sref->type == REF_VAL_STR);

  ZValue *tvalue = object_prop_get(vm, tref->data.obj, "value");
  ZValue *svalue = object_prop_get(vm, sref->data.obj, "value");

  if (tvalue == NULL) {
    printf("[WARN]: concat target string can't be null.");
    return;
  }
  if (svalue == NULL) { // no need continue
    return;
  }
  assert(tvalue->type == VAL_REF && svalue->type == VAL_REF);
  ZRefValue *tvalue_ref = (ZRefValue *)tvalue->data.ptr;
  ZRefValue *svalue_ref = (ZRefValue *)svalue->data.ptr;
  assert(tvalue_ref->type == REF_VAL_STR_RAW &&
         svalue_ref->type == REF_VAL_STR_RAW);
  const char *target_src = tvalue_ref->data.rstring->raw;
  const char *src_str = svalue_ref->data.rstring->raw;
  int count = strlen(target_src) + strlen(src_str) + 1;
  char *new_str = (char *)malloc(sizeof(char) * count);
  memcpy(new_str, target_src, strlen(target_src));
  memcpy(new_str + strlen(target_src), src_str, strlen(src_str));
  *(new_str + count) = '\0';
  free(target_src);
  tvalue_ref->data.rstring->raw = new_str;
  deallocator_data(vm->mm, tvalue);
  deallocator_data(vm->mm, svalue);
}

void native_string_substr(VM *vm) {
  ZValue *str = vm->stacks[vm->sp - 3];
  ZValue *start = vm->stacks[vm->sp];
  ZValue *size = vm->stacks[vm->sp - 1];
  assert(str->type == VAL_REF);
  assert(start->type == VAL_INT);
  assert(size->type == VAL_INT);
  ZRefValue *ref = (ZRefValue *)str->data.ptr;
  assert(ref->type == REF_VAL_STR);
  ZValue *src_value = object_prop_get(vm, ref->data.obj, "value");
  if (src_value == NULL) {
    ZValue *ret = allocator_data(vm->mm, VAL_REF, REF_VAL_STR,
                                 &(AllocatorParams){.str.str_length = 1});
    map_insert(vm->registers, "ei", ret);
    return;
  }
  assert(src_value->type == VAL_REF);
  ZRefValue *src_value_ref = (ZRefValue *)src_value->data.ptr;
  size_t raw_size = strlen(src_value_ref->data.rstring->raw);
  int start_index = start->data.i_val;
  int size_value = size->data.i_val;
  if (start_index < 0) {
    start_index = 0;
  }
  if (start_index >= raw_size) {
    map_insert(vm->registers, "ei",
               allocator_data(vm->mm, VAL_REF, REF_VAL_STR,
                              &(AllocatorParams){.str.str_length = 1}));
    deallocator_data(vm->mm, src_value);
    return;
  }
  if ((start_index + size_value - 1) >= raw_size) {
    size_value = raw_size - start_index;
  }
  ZValue *ret =
      allocator_data(vm->mm, VAL_REF, REF_VAL_STR,
                     &(AllocatorParams){.str.str_length = size_value + 1});
  assert(ret != NULL && ret->type == VAL_REF);
  ZRefValue *ret_ref = (ZRefValue *)ret->data.ptr;
  ZValue *ret_value = object_prop_get(vm, ret_ref->data.obj, "value");
  assert(ret_value != NULL && ret_value->type == VAL_REF);
  ZRefValue *ret_value_ref = (ZRefValue *)ret_value->data.ptr;
  assert(ret_value_ref->type == REF_VAL_STR_RAW);
  memcpy(ret_value_ref->data.rstring->raw,
         (src_value_ref->data.rstring->raw) + start_index, size_value);

  map_insert(vm->registers, "ei", ret);
  deallocator_data(vm->mm, src_value);
  deallocator_data(vm->mm, ret_value);
}

void native_vector_insert(VM *vm) {
  ZValue *target = vm->stacks[vm->sp];
  ZValue *index = vm->stacks[vm->sp - 1];
  ZValue *src = vm->stacks[vm->sp - 2];
  assert(target->type == VAL_REF);
  ZRefValue *ref = (ZRefValue *)target->data.ptr;
  assert(ref->type == REF_VAL_ARR);
  assert(index->type == VAL_INT);
  if (ref->data.arr->length < ref->data.arr->capacity) {
    for (int i = ref->data.arr->length; i > index->data.i_val; i--) {
      ref->data.arr->elements[i] = ref->data.arr->elements[i - 1];
    }
    ref->data.arr->elements[index->data.i_val] = copy(vm, src);
    ref->data.arr->length += 1;
    return;
  }
  int raw_capacity = ref->data.arr->capacity;
  ref->data.arr->capacity *= 2;
  ZValue **elements = ref->data.arr->elements;
  ref->data.arr->elements =
      (ZValue **)malloc(sizeof(ZValue *) * ref->data.arr->capacity);
  memory_allocator(vm->mm, sizeof(ZValue *) * ref->data.arr->capacity);
  for (int i = 0; i < index->data.i_val; i++) {
    ref->data.arr->elements[i] = elements[i];
  }
  for (int i = ref->data.arr->length; i > index->data.i_val; i--) {
    ref->data.arr->elements[i] = elements[i - 1];
  }
  ref->data.arr->elements[index->data.i_val] = copy(vm, src);
  ref->data.arr->length += 1;
  free(elements);
  memory_deallocator(vm->mm, sizeof(ZValue *) * raw_capacity);
}

void native_vector_get(VM *vm) {
  ZValue *arr = vm->stacks[vm->sp];
  ZValue *index = vm->stacks[vm->sp - 1];
  assert(arr->type == VAL_REF);
  ZRefValue *ref = (ZRefValue *)arr->data.ptr;
  assert(ref->type == REF_VAL_ARR);
  assert(index->type == VAL_INT);
  assert(index->data.i_val < ref->data.arr->length);
  ZValue *result_value = ref->data.arr->elements[index->data.i_val];
  ZValue *result = NULL;
  if (result_value->type != VAL_REF) {
    result = allocator_data(vm->mm, result_value->type, 0, NULL);
    result->data = result_value->data;
  } else {
    result = allocator_data(vm->mm, VAL_REF, 0,
                            &(AllocatorParams){.ref.is_shell = true});
    result->data.ptr = result_value->data.ptr;
    ((ZRefValue *)result_value->data.ptr)->ref_count += 1;
  }
  map_insert(vm->registers, "ei", result);
}

void native_panic(VM *vm) {
  ZValue *code = vm->stacks[vm->sp];
  ZValue *msg = vm->stacks[vm->sp - 1];
  assert(code->type == VAL_INT);
  assert(msg->type == VAL_REF);
  ZRefValue *msg_ref = (ZRefValue *)msg->data.ptr;
  assert(msg_ref->type = REF_VAL_STR);
  ZValue *msg_value = object_prop_get(vm, msg_ref->data.obj, "value");
  if (msg_value != NULL) {
    ZRefValue *msg_value_ref = (ZRefValue *)msg_value->data.ptr;
    if (msg_value_ref != NULL && msg_value_ref->type == REF_VAL_STR_RAW) {
      printf("[PANIC]:%s\n", msg_value_ref->data.rstring->raw);
    } else {
      printf("[PANIC]\n");
    }
    deallocator_data(vm->mm, msg_value);
    exit(code->data.i_val);
  }
  printf("[PANIC]\n");
  exit(code->data.i_val);
}

void native_open(VM *vm) {
  ZValue *v = vm->stacks[vm->sp];
  assert(v->type == VAL_REF);
  ZRefValue *v_ref = (ZRefValue *)v->data.ptr;
  assert(v_ref->type == REF_VAL_STR);
  ZValue *v_value = object_prop_get(vm, v_ref->data.obj, "value");
  if (v_value == NULL) {
    ZValue *f = allocator_data(vm->mm, VAL_INT, 0, NULL);
    f->data.i_val = -1;
    map_insert(vm->registers, "ei", f);
    return;
  }
  ZRefValue *v_value_ref = (ZRefValue *)v_value->data.ptr;
  assert(v_value_ref->type == REF_VAL_STR_RAW);
  const char *file_name = v_value_ref->data.rstring->raw;
  assert(file_name != NULL);
  if (file_name == NULL || strlen(file_name) == 0) {
    map_insert(vm->registers, "ei", allocator_data(vm->mm, VAL_NULL, 0, NULL));
    deallocator_data(vm->mm, v_value);
    return;
  }
  int fd = open(file_name, O_RDONLY | O_CLOEXEC);
  if (fd == -1) {
    map_insert(vm->registers, "ei", allocator_data(vm->mm, VAL_NULL, 0, NULL));
    deallocator_data(vm->mm, v_value);
    return;
  }
  ZValue *f = allocator_data(vm->mm, VAL_INT, 0, NULL);
  f->data.i_val = fd;
  map_insert(vm->registers, "ei", f);
  deallocator_data(vm->mm, v_value);
}

void native_read(VM *vm) {
  ZValue *v = vm->stacks[vm->sp];
  if (v->type == VAL_NULL) {
    map_insert(vm->registers, "ei",
               allocator_data(vm->mm, VAL_REF, REF_VAL_STR,
                              &(AllocatorParams){.str.str_length = 1}));
    return;
  }
  assert(v->type == VAL_REF);
  ZRefValue *obj_ref = (ZRefValue *)v->data.ptr;
  assert(obj_ref->type == REF_VAL_OBJ);
  ZObject *obj = obj_ref->data.obj;
  ZValue *fdv = NULL;
  for (int i = 0; i < obj->count; i++) {
    if (strcmp("fd", obj->entries[i].key) == 0) {
      fdv = obj->entries[i].value;
      break;
    }
  }
  if (fdv == NULL) {
    map_insert(vm->registers, "ei",
               allocator_data(vm->mm, VAL_REF, REF_VAL_STR,
                              &(AllocatorParams){.str.str_length = 1}));
    return;
  }
  assert(fdv->type == VAL_INT);
  int fd = fdv->data.i_val;
  off_t file_size = lseek(fd, 0, SEEK_END);
  if (file_size == -1) {
    map_insert(vm->registers, "ei",
               allocator_data(vm->mm, VAL_REF, REF_VAL_STR,
                              &(AllocatorParams){.str.str_length = 1}));
    return;
  }
  lseek(fd, 0, SEEK_SET);
  char *file_content = (char *)malloc(file_size + 1);
  if (file_content == NULL) {
    map_insert(vm->registers, "ei",
               allocator_data(vm->mm, VAL_REF, REF_VAL_STR,
                              &(AllocatorParams){.str.str_length = 1}));
    return;
  }
  ssize_t read_bytes = read(fd, file_content, file_size);
  if (read_bytes == -1) {
    free(file_content);
    map_insert(vm->registers, "ei",
               allocator_data(vm->mm, VAL_REF, REF_VAL_STR,
                              &(AllocatorParams){.str.str_length = 1}));
    return;
  }
  file_content[read_bytes] = '\0';

  ZValue *sf =
      allocator_data(vm->mm, VAL_REF, REF_VAL_STR,
                     &(AllocatorParams){.str.str_length = file_size + 1});
  ZValue *sf_v =
      object_prop_get(vm, ((ZRefValue *)sf->data.ptr)->data.obj, "value");
  assert(sf_v != NULL);
  ZRefValue *sf_v_ref = (ZRefValue *)sf_v->data.ptr;
  assert(sf_v_ref->type == REF_VAL_STR_RAW);
  memcpy(sf_v_ref->data.rstring->raw, file_content, file_size + 1);
  free(file_content);
  map_insert(vm->registers, "ei", sf);
  deallocator_data(vm->mm, sf_v);
}

void native_close(VM *vm) {
  ZValue *v = vm->stacks[vm->sp];
  if (v->type == VAL_NULL) {
    map_insert(vm->registers, "ei", allocator_data(vm->mm, VAL_NULL, 0, NULL));
    return;
  }
  assert(v->type == VAL_REF);
  ZRefValue *obj_ref = (ZRefValue *)v->data.ptr;
  assert(obj_ref->type == REF_VAL_OBJ);
  ZObject *obj = obj_ref->data.obj;
  ZValue *fdv = NULL;
  for (int i = 0; i < obj->count; i++) {
    if (strcmp("fd", obj->entries[i].key) == 0) {
      fdv = obj->entries[i].value;
      break;
    }
  }
  if (fdv == NULL) {
    map_insert(vm->registers, "ei", allocator_data(vm->mm, VAL_NULL, 0, NULL));
    return;
  }
  assert(fdv->type == VAL_INT);
  int fd = fdv->data.i_val;
  int result = close(fd);
  // Avoid accidental reuse of file descriptors after closing.
  if (result == -1) {
    map_insert(vm->registers, "ei", allocator_data(vm->mm, VAL_NULL, 0, NULL));
    return;
  }

  fdv->data.i_val = -1;
  map_insert(vm->registers, "ei", allocator_data(vm->mm, VAL_NULL, 0, NULL));
}

void call_builtin_function(VM *vm, ZFunction *func) {
  assert(func->is_builtin);
  if (strcmp(func->name, "native_println") == 0) {
    native_println(vm);
  } else if (strcmp(func->name, "native_new_array") == 0) {
    native_new_array(vm);
  } else if (strcmp(func->name, "native_new_object") == 0) {
    native_new_object(vm);
  } else if (strcmp(func->name, "native_print") == 0) {
    native_print(vm);
  } else if (strcmp(func->name, "native_open") == 0) {
    native_open(vm);
  } else if (strcmp(func->name, "native_read") == 0) {
    native_read(vm);
  } else if (strcmp(func->name, "native_close") == 0) {
    native_close(vm);
  } else if (strcmp(func->name, "__hash__") == 0) {
    zero_hash(vm);
  } else if (strcmp(func->name, "native_string_len") == 0) {
    native_string_len(vm);
  } else if (strcmp(func->name, "native_string_substr") == 0) {
    native_string_substr(vm);
  } else if (strcmp(func->name, "native_string_equal") == 0) {
    native_string_equal(vm);
  } else if (strcmp(func->name, "native_string_concat") == 0) {
    native_string_concat(vm);
  } else if (strcmp(func->name, "native_vector_insert") == 0) {
    native_vector_insert(vm);
  } else if (strcmp(func->name, "native_vector_get") == 0) {
    native_vector_get(vm);
  } else if (strcmp(func->name, "native_panic") == 0) {
    native_panic(vm);
  } else {
    assert(false);
  }
}

bool is_true(VM *vm, ZValue *value) {
  switch (value->type) {
  case VAL_BOOL:
    return value->data.b_val;
  case VAL_FLOAT:
    return value->data.f_val;
  case VAL_INT:
    return value->data.i_val;
  case VAL_CHAR:
    return value->data.c_val;
  case VAL_NULL:
    return false;
  default:
    return is_true(vm, (ZValue *)(value->data.ptr));
  }
}

void JUMP_INST_RUN(VM *vm, ZValue *value) {
  assert(value->type == VAL_INT);
  vm->cur_context->pc += value->data.i_val;
  if (value->data.i_val < 0) {
    vm->cur_context->pc -= 1;
  }
}

void JF_INST_RUN(VM *vm, ZValue *value) {
  assert(value->type == VAL_INT);
  if (!is_true(vm, vm->stacks[vm->sp--])) {
    vm->cur_context->pc += value->data.i_val;
  }
  deallocator_data(vm->mm, vm->stacks[vm->sp + 1]);
}

ZRefValue *copy_ref(VM *vm, ZRefValue *src) {
  switch (src->type) {
  case REF_VAL_ARR: {
    ZArray *raw = src->data.arr;
    ZRefValue *data = allocator_ref_data(
        vm->mm, REF_VAL_ARR, &(AllocatorParams){.arr.elem_count = raw->length});
    ZArray *arr = data->data.arr;
    for (int i = 0; i < arr->length; i++) {
      ZValue *src = raw->elements[i];
      arr->elements[i] = copy(vm, src);
    }
    return data;
  } break;
  case REF_VAL_STR:
  case REF_VAL_OBJ: {
    ZObject *raw = src->data.obj;
    ZRefValue *data = allocator_ref_data(
        vm->mm, src->type, &(AllocatorParams){.obj.kv_count = raw->count});
    ZObject *obj = data->data.obj;
    for (int i = 0; i < obj->count; i++) {
      obj->entries[i].key = raw->entries[i].key;
      obj->entries[i].value = copy(vm, raw->entries[i].value);
    }
    return data;
  } break;
  case REF_VAL_FUNC: {
    ZRefValue *data = allocator_ref_data(
        vm->mm, REF_VAL_FUNC, &(AllocatorParams){.func.new_build = false});
    // ZFunction don't copy
    data->data.func = src->data.func;
    return data;
  } break;
  case REF_VAL_STR_RAW: {
    ZRefValue *data = allocator_ref_data(
        vm->mm, REF_VAL_STR_RAW,
        &(AllocatorParams){.str.str_length =
                               strlen(src->data.rstring->raw) + 1});
    memcpy(data->data.rstring->raw, src->data.rstring->raw,
           strlen(src->data.rstring->raw) + 1);
    return data;
  } break;
  default:
    assert(false);
    break;
  }
}

ZValue *str_index_to_ref_sting(VM *vm, ZValue *src) {}

ZValue *copy(VM *vm, ZValue *src) {
  switch (src->type) {
  case VAL_REF: {
    ZValue *ref = allocator_data(vm->mm, VAL_REF, 0,
                                 &(AllocatorParams){.ref.is_shell = true});
    ref->data.ptr = copy_ref(vm, (ZRefValue *)(src->data.ptr));
    return ref;
  } break;
  case VAL_INT:
  case VAL_BOOL:
  case VAL_CHAR:
  case VAL_FLOAT:
  case VAL_NULL:
  case VAL_OFFSET:
  case VAL_REGISTER: {
    ZValue *v = allocator_data(vm->mm, src->type, 0, NULL);
    v->data = src->data;
    return v;
  }
  default:
    assert(false);
  }
}

void SET_INST_RUN(VM *vm, ZValue *value) {
  assert(vm->sp >= 2);
  ZValue *ref = vm->stacks[vm->sp - 2];
  ZValue *prop = vm->stacks[vm->sp - 1];
  ZValue *v = vm->stacks[vm->sp];
  assert(ref->type == VAL_REF);
  assert(prop->type == VAL_REF);
  ZRefValue *prop_ref = (ZRefValue *)prop->data.ptr;
  assert(prop_ref->type == REF_VAL_STR);
  ZRefValue *obj_wrapper = (ZRefValue *)ref->data.ptr;
  assert(obj_wrapper->type == REF_VAL_OBJ);
  ZObject *obj = obj_wrapper->data.obj;
  ZValue *key_value = object_prop_get(vm, prop_ref->data.obj, "value");
  assert(key_value != NULL);
  ZRefValue *key_value_ref = (ZRefValue *)key_value->data.ptr;
  const char *key = key_value_ref->data.rstring->raw;
  for (int i = 0; i < obj->count; i++) {
    if (strcmp(obj->entries[i].key, key) == 0) {
      if (v->type == VAL_REF) {
        ZValue *nv = allocator_data(vm->mm, VAL_REF, 0,
                                    &(AllocatorParams){.ref.is_shell = true});
        nv->data.ptr = v->data.ptr;
        ((ZRefValue *)nv->data.ptr)->ref_count += 1;
        obj->entries[i].value = nv;
      } else {
        ZValue *nv = allocator_data(vm->mm, v->type, 0, NULL);
        nv->type = v->type;
        nv->data = v->data;
        obj->entries[i].value = nv;
      }
      deallocator_data(vm->mm, v);
      vm->sp -= 3;
      deallocator_data(vm->mm, key_value);
      return;
    }
  }
  deallocator_data(vm->mm, key_value);
}

const char *get_type_str(int type) {
  switch (type) {
  case VAL_INT:
    return "int";
  case VAL_FLOAT:
    return "float";
  case VAL_BOOL:
    return "bool";
  case VAL_REF:
    return "object";
  default:
    return "unknown";
  }
}

void CHECK_INST_RUN(VM *vm, ZValue *value) {
  ZValue *need_check = vm->stacks[vm->sp];
  int type = value->data.i_val;
  if (type == TYPE_INT) {
    ZERO_ASSERT(need_check->type == VAL_INT, "expected type is int, but get %s",
                get_type_str(need_check->type));
  } else if (type == TYPE_FLOAT) {
    ZERO_ASSERT(need_check->type == VAL_FLOAT,
                "expected type is float, but get %s",
                get_type_str(need_check->type));
  } else if (type == TYPE_BOOL) {
    ZERO_ASSERT(need_check->type == VAL_BOOL,
                "expected type is bool, but get %s",
                get_type_str(need_check->type));
  } else if (type == TYPE_STR) {
    if (need_check->type == VAL_REF) {
      ZRefValue *ref = (ZRefValue *)need_check->data.ptr;
      if (ref->type == REF_VAL_STR) {
        return;
      }
    }
    ZERO_ASSERT(false, "expected type is string, but get %s",
                get_type_str(need_check->type));
  } else if (type == TYPE_OBJ || type == TYPE_CUSTOM) {
    ZERO_ASSERT(need_check->type == VAL_REF,
                "expected type is object, but get %s",
                get_type_str(need_check->type));
  } else {
    ZERO_ASSERT(type > TYPE_UNKNOWN && type <= TYPE_CUSTOM, "TYPE unknown");
  }
}

void COPY_INST_RUN(VM *vm, ZValue *value) {
  ZValue *src = vm->stacks[vm->sp];
  assert(src->type == VAL_REF);
  // vm->stacks[++vm->sp] = src;//
  ZValue *target = allocator_data(vm->mm, VAL_REF, 0,
                                  &(AllocatorParams){.ref.is_shell = true});
  target->data.ptr = src->data.ptr;
  ((ZRefValue *)target->data.ptr)->ref_count += 1;
  vm->stacks[++vm->sp] = target;
}

ZValue *object_prop_get(VM *vm, ZObject *raw_obj, const char *key) {
  for (int i = 0; i < raw_obj->count; i++) {
    if (strcmp(raw_obj->entries[i].key, key) == 0) {
      ZValue *src = raw_obj->entries[i].value;
      ZValue *target = NULL;
      if (src->type == VAL_REF) {
        target = allocator_data(vm->mm, VAL_REF, 0,
                                &(AllocatorParams){.ref.is_shell = true});
        target->data.ptr = src->data.ptr;
        ((ZRefValue *)target->data.ptr)->ref_count += 1;
      } else {
        target = allocator_data(vm->mm, src->type, 0, NULL);
        target->data = src->data;
      }
      return target;
    }
  }
  return NULL;
}

void GET_INST_RUN(VM *vm, ZValue *value) {
  ZValue *obj_ref = vm->stacks[vm->sp - 1];
  ZValue *prop = vm->stacks[vm->sp];
  assert(prop->type == VAL_REF);
  ZRefValue *prop_ref = (ZRefValue *)prop->data.ptr;
  assert(prop_ref->type == REF_VAL_STR);
  ZValue *prop_value = object_prop_get(vm, prop_ref->data.obj, "value");
  assert(prop_value != NULL);
  ZRefValue *prop_value_ref = (ZRefValue *)prop_value->data.ptr;
  const char *key = prop_value_ref->data.rstring->raw;
  assert(obj_ref->type == VAL_REF);
  ZRefValue *obj = (ZRefValue *)obj_ref->data.ptr;
  assert(obj->type == REF_VAL_OBJ || obj->type == REF_VAL_STR);
  ZObject *raw_obj = obj->data.obj;
  ZObject *re = object_prop_get(vm, raw_obj, key);
  if (re == NULL) {
    re = object_prop_get(vm, raw_obj->prototype, key);
  }
  if (re != NULL) {
    deallocator_data(vm->mm, obj_ref);
    deallocator_data(vm->mm, prop);
    deallocator_data(vm->mm, prop_value);
    vm->stacks[--(vm->sp)] = re;
  } else {
    assert(false);
  }
}

void CALL_INST_RUN(VM *vm, ZValue *value) {
  // call from stack, eg: CALL -1
  if (value->type == VAL_INT) {
    value = vm->stacks[vm->sp + value->data.i_val];
  }
  assert(value->type == VAL_REF);
  ZRefValue *ref = (ZRefValue *)value->data.ptr;
  assert(ref->type == REF_VAL_FUNC);
  ZFunction *func = ref->data.func;
  Runnable *runnable = (Runnable *)malloc(sizeof(Runnable));
  runnable->ctx = new_context();
  runnable->func = func;
  bool vm_stack_is_empty = vm->sp < 0;
  runnable->ctx->bp = vm_stack_is_empty ? -1 : vm->sp;
  runnable->ctx->parent = vm->cur_context;
  vm->cur_context = runnable->ctx;
  if (func->is_builtin) {
    call_builtin_function(vm, func);
    vm->cur_context = runnable->ctx->parent;
    free_runnable(runnable);
    return;
  }
  struct Vec *code = func->instructions;
  for (; runnable->ctx->pc < code->count; runnable->ctx->pc++) {
    INSTRUCTION *inst = (INSTRUCTION *)code->get(code, runnable->ctx->pc);
    switch (inst->code) {
    case I_PUSH:
      PUSH_INST_RUN(vm, (ZValue *)inst->v);
      break;
    case I_STORE: {
      STORE_INST_RUN(vm, (ZValue *)inst->v);
      if (inst->v->type == VAL_REGISTER &&
          strcmp((const char *)(inst->v->data.ptr), "ei") == 0) {
        runnable->ctx->pc = code->count;
      }
    } break;
    case I_LOAD:
      LOAD_INST_RUN(vm, (ZValue *)inst->v);
      break;
    case I_ADD:
      ADD_INST_RUN(vm, (ZValue *)inst->v);
      break;
    case I_MINUS:
      MINUS_INST_RUN(vm, (ZValue *)inst->v);
      break;
    case I_MULTI:
      MULTI_INST_RUN(vm, (ZValue *)inst->v);
      break;
    case I_DIV:
      DIV_INST_RUN(vm, (ZValue *)inst->v);
      break;
    case I_CALL: {
      vm->cur_context = runnable->ctx->parent;
      CALL_INST_RUN(vm, (ZValue *)inst->v);
      vm->cur_context = runnable->ctx;
    } break;
    case I_GET:
      GET_INST_RUN(vm, inst->v);
      break;
    case I_G:
    case I_GE:
    case I_L:
    case I_LE:
    case I_NEGATE:
    case I_NOT:
    case I_E:
    case I_NE:
    case I_AND:
    case I_OR:
      LOGIC_INST_RUN(vm, inst->v, inst->code);
      break;
    case I_ASSIGN:
      ASSIGN_INST_RUN(vm, (ZValue *)inst->v);
      break;
    case I_FREE:
      FREE_INST_RUN(vm, inst->v);
      break;
    case I_JUMP:
      JUMP_INST_RUN(vm, inst->v);
      break;
    case I_JF:
      JF_INST_RUN(vm, inst->v);
      break;
    case I_SET:
      SET_INST_RUN(vm, inst->v);
      break;
    case I_CHECK:
      CHECK_INST_RUN(vm, inst->v);
      break;
    case I_COPY:
      COPY_INST_RUN(vm, inst->v);
      break;
    default:
      assert(false);
    }
  }
  vm->cur_context = runnable->ctx->parent;
  /*
  It's very important!
  Very important: The FREE instruction cleans up parameters from the source
  function stack, but you are responsible for clearing the data inside the
  sub-function stack yourself.
  */
  int cur_sp = vm->sp;
  if (vm_stack_is_empty) {
    vm->sp = -1;
  } else {
    vm->sp = runnable->ctx->bp;
  }
  free_runnable(runnable);
  for (int i = vm->sp + 1; i <= cur_sp; i++) {
    deallocator_data(vm->mm, vm->stacks[i]);
  }
}

void __INIT__RUN(VM *vm, ZValue *value) {
  assert(value->type == VAL_REF);
  ZRefValue *ref = (ZRefValue *)value->data.ptr;
  assert(ref->type == REF_VAL_FUNC);
  ZFunction *func = ref->data.func;

  Runnable *runnable = (Runnable *)malloc(sizeof(Runnable));
  runnable->ctx = new_context();
  runnable->func = func;
  runnable->ctx->bp = 0;
  runnable->ctx->parent = vm->cur_context;
  vm->cur_context = runnable->ctx;

  struct Vec *code = func->instructions;
  for (; runnable->ctx->pc < code->count; runnable->ctx->pc++) {
    INSTRUCTION *inst = (INSTRUCTION *)code->get(code, runnable->ctx->pc);
    switch (inst->code) {
    case I_PUSH:
      PUSH_INST_RUN(vm, (ZValue *)inst->v);
      break;
    case I_STORE: {
      STORE_INST_RUN(vm, (ZValue *)inst->v);
      if (inst->v->type == VAL_REGISTER &&
          strcmp((const char *)(inst->v->data.ptr), "ei") == 0) {
        runnable->ctx->pc = code->count;
      }
    } break;
    case I_LOAD:
    case I_ADD:
    case I_MINUS:
    case I_MULTI:
    case I_DIV:
    case I_CALL:
    case I_GET:
    case I_G:
    case I_GE:
    case I_L:
    case I_LE:
    case I_NEGATE:
    case I_NOT:
    case I_E:
    case I_NE:
    case I_ASSIGN:
    case I_FREE:
    case I_JUMP:
    case I_JF:
    case I_CHECK:
    case I_COPY:
    case I_AND:
    case I_OR:
    default:
      assert(false);
    }
  }
  vm->cur_context = runnable->ctx->parent;
}

int VM_Run(VM *vm) {
  for (int i = vm->globals->count - 1; i >= 0; i--) {
    __INIT__RUN(vm, vm->globals->get(vm->globals, i));
  }
  CALL_INST_RUN(vm, vm->entry);
}

VM *new_vm() {
  VM *vm = (VM *)malloc(sizeof(VM));
  vm->entry = NULL;
  vm->root_context = new_context();
  vm->cur_context = vm->root_context;
  vm->ready_link = new_map();
  vm->globals = new_vec();
  vm->sp = -1;
  vm->registers = new_map();
  vm->symbols = new_map();
  vm->cvalues = new_vec();
  vm->mm = new_mm();
  map_insert(vm->registers, "ei", NULL);
  return vm;
}

INSTRUCTION *NEW_STORE_INSTRUCTION(VM *vm, const char *value) {
  if (value[0] == '[') {
    int full_len = strlen(value);
    assert(full_len > 2);
    int content_len = full_len - 2;
    ZValue *zv = allocator_data(
        vm->mm, VAL_REGISTER, 0,
        &(AllocatorParams){.reg.register_name_size = content_len + 1});
    memcpy(zv->data.ptr, value + 1, content_len);
    *((char *)zv->data.ptr + content_len) = '\0';
    return new_inst(I_STORE, zv);
  } else if (value[0] == '#') {
    int offset = (int)strtod(value + 1, NULL);
    ZValue *zv = allocator_data(vm->mm, VAL_OFFSET, 0, NULL);
    zv->data.i_val = offset;
    return new_inst(I_STORE, zv);
  } else {
    int offset = (int)strtod(value, NULL);
    ZValue *zv = allocator_data(vm->mm, VAL_INT, 0, NULL);
    zv->data.i_val = offset;
    return new_inst(I_STORE, zv);
  }
}

INSTRUCTION *NEW_LOAD_INSTRUCTION(VM *vm, const char *value) {
  if (value[0] == '#') {
    int offset = (int)strtod(value + 1, NULL);
    ZValue *zv = allocator_data(vm->mm, VAL_OFFSET, 0, NULL);
    zv->data.i_val = offset;
    return new_inst(I_LOAD, zv);
  } else if (value[0] == '[') {
    int full_len = strlen(value);
    assert(full_len > 2);
    int content_len = full_len - 2;
    ZValue *zv = allocator_data(
        vm->mm, VAL_REGISTER, 0,
        &(AllocatorParams){.reg.register_name_size = content_len + 1});
    memcpy(zv->data.ptr, value + 1, content_len);
    *((char *)zv->data.ptr + content_len) = '\0';
    return new_inst(I_LOAD, zv);
  } else if (value[0] == '.') {
    MapPair *pair = map_get(vm->symbols, value + 1);
    if (pair != NULL) {
      ZValue *f = (ZValue *)(pair->value);
      return new_inst(I_LOAD, f);
    } else {
      ZValue *v = allocator_data(vm->mm, VAL_REF, REF_VAL_FUNC,
                                 &(AllocatorParams){.func.new_build = false});
      INSTRUCTION *inst = new_inst(I_LOAD, v);
      MapPair *pair = map_get(vm->ready_link, value);
      if (pair == NULL) {
        struct Vec *store = new_vec();
        store->push(store, inst);
        map_insert(vm->ready_link, value + 1, store);
        return inst;
      }
      struct Vec *store = pair->value;
      store->push(store, inst);
      return inst;
    }
  } else {
    int offset = (int)strtod(value, NULL);
    ZValue *zv = allocator_data(vm->mm, VAL_INT, 0, NULL);
    zv->data.i_val = offset;
    return new_inst(I_LOAD, zv);
  }
}

INSTRUCTION *NEW_PUSH_D_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = allocator_data(vm->mm, VAL_INT, 0, NULL);
  v->data.i_val = (int)strtod(value, NULL);
  return new_inst(I_PUSH, v);
}

INSTRUCTION *NEW_PUSH_F_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = allocator_data(vm->mm, VAL_FLOAT, 0, NULL);
  v->data.f_val = strtod(value, NULL);
  return new_inst(I_PUSH, v);
}
// do nothing now
INSTRUCTION *NEW_PUSH_B_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = allocator_data(vm->mm, VAL_BOOL, 0, NULL);
  v->data.b_val = (int)strtod(value, NULL) == 1;
  return new_inst(I_PUSH, v);
}

INSTRUCTION *NEW_PUSH_S_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v =
      allocator_data(vm->mm, VAL_REF, REF_VAL_STR,
                     &(AllocatorParams){.str.str_length = strlen(value) + 1});
  vm->cvalues->push(vm->cvalues, value);
  ZRefValue *ref = (ZRefValue *)v->data.ptr;
  ZValue *ref_value = object_prop_get(vm, ref->data.obj, "value");
  ZRefValue *ref_value_ref = (ZRefValue *)ref_value->data.ptr;
  memcpy(ref_value_ref->data.rstring->raw, value, strlen(value));
  ref_value_ref->data.rstring->raw[strlen(value)] = '\0';
  deallocator_data(vm->mm, ref_value);
  return new_inst(I_PUSH, v);
}

INSTRUCTION *NEW_PUSH_N_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = allocator_data(vm->mm, VAL_NULL, 0, NULL);
  return new_inst(I_PUSH, v);
}

INSTRUCTION *NEW_CALL_INSTRUCTION(VM *vm, const char *value) {
  if (value[0] != '-') {
    ZValue *v = allocator_data(vm->mm, VAL_REF, REF_VAL_FUNC,
                               &(AllocatorParams){.func.new_build = false});
    INSTRUCTION *inst = new_inst(I_CALL, v);
    MapPair *pair = map_get(vm->ready_link, value);
    if (pair == NULL) {
      struct Vec *store = new_vec();
      store->push(store, inst);
      map_insert(vm->ready_link, value, store);
      return inst;
    }
    struct Vec *store = pair->value;
    store->push(store, inst);
    return inst;
  } else {
    ZValue *v = allocator_data(vm->mm, VAL_INT, 0, NULL);
    v->data.i_val = (int)strtod(value, NULL);
    return new_inst(I_CALL, v);
  }
}
INSTRUCTION *NEW_ADD_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_ADD, NULL);
}
INSTRUCTION *NEW_MINUS_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_MINUS, NULL);
}
INSTRUCTION *NEW_MULTI_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_MULTI, NULL);
}
INSTRUCTION *NEW_DIV_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_DIV, NULL);
}

INSTRUCTION *NEW_G_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_G, NULL);
}
INSTRUCTION *NEW_GE_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_GE, NULL);
}
INSTRUCTION *NEW_L_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_L, NULL);
}
INSTRUCTION *NEW_LE_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_LE, NULL);
}
INSTRUCTION *NEW_NEGATE_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_NEGATE, NULL);
}
INSTRUCTION *NEW_NOT_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_NOT, NULL);
}
INSTRUCTION *NEW_EQUAL_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_E, NULL);
}
INSTRUCTION *NEW_NOT_EQUAL_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_NE, NULL);
}

INSTRUCTION *NEW_ASSIGN_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_ASSIGN, NULL);
}

INSTRUCTION *NEW_FREE_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = allocator_data(vm->mm, VAL_INT, 0, NULL);
  v->data.i_val = (int)strtod(value, NULL);
  return new_inst(I_FREE, v);
}

INSTRUCTION *NEW_JUMP_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = allocator_data(vm->mm, VAL_INT, 0, NULL);
  v->data.i_val = (int)strtod(value, NULL);
  return new_inst(I_JUMP, v);
}

INSTRUCTION *NEW_JF_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = allocator_data(vm->mm, VAL_INT, 0, NULL);
  v->data.i_val = (int)strtod(value, NULL);
  return new_inst(I_JF, v);
}

INSTRUCTION *NEW_GET_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_GET, NULL);
}

INSTRUCTION *NEW_SET_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_SET, NULL);
}

INSTRUCTION *NEW_CHECK_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = allocator_data(vm->mm, VAL_INT, 0, NULL);
  v->data.i_val = (int)strtod(value, NULL);
  return new_inst(I_CHECK, v);
}
INSTRUCTION *NEW_COPY_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_COPY, NULL);
}

INSTRUCTION *NEW_AND_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_AND, NULL);
}
INSTRUCTION *NEW_OR_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_OR, NULL);
}

const char *append_suffix(const char *m) {
  int len = strlen(m);
  char *source_file_name = (char *)malloc(sizeof(char) * (len + 3));
  memcpy(source_file_name, m, len);
  source_file_name[len] = '.';
  source_file_name[len + 1] = 'z';
  source_file_name[len + 2] = '\0';
  return source_file_name;
}

Map *programs = NULL;

void zero_parser(const char *file_name) {
  if (programs == NULL) {
    programs = new_map();
  }
  if (GLOBAL_PARSER == NULL) {
    GLOBAL_PARSER = parser_init();
  }
  GLOBAL_PARSER = parser_reload(GLOBAL_PARSER, file_name);
  struct syntax_program *program = parser_program(GLOBAL_PARSER);
  map_insert(programs, file_name, program);
  for (int i = 0; i < program->include_paths->count; i++) {
    zero_parser(
        append_suffix(program->include_paths->get(program->include_paths, i)));
  }
}

void zero_compile(const char *file_name) {
  struct instruction_store *st = NULL;
  if (root_instruction_store == NULL) {
    ROOT_INSTRUCTION_INIT();
    cur_instruction_store = root_instruction_store;
  } else {
    st = new_instruction_store();
    st->parent = cur_instruction_store;
    cur_instruction_store->childs->push(cur_instruction_store->childs, st);
    cur_instruction_store = st;
  }

  MapPair *pair = map_get(programs, file_name);
  if (pair == NULL) {
    assert(false);
  }
  struct syntax_program *program = (struct syntax_program *)pair->value;
  GET_INSTRUCTION_STORE()->fd =
      open(target_filename(file_name), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  program_visitor(program);
  instruction_to_file();
  for (int i = 0; i < program->include_paths->count; i++) {
    zero_compile(
        append_suffix(program->include_paths->get(program->include_paths, i)));
    if (st == NULL) {
      cur_instruction_store = root_instruction_store;
    } else {
      cur_instruction_store = st->parent;
    }
  }
}

void compile(const char *file_name) {
  zero_parser(file_name);
  zero_compile(file_name);
}

void run() {
  VM *v = new_vm();
  VM_Init(v);
  VM_Run(v);
}

#endif
#endif
