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

struct token {
  TOKEN_TYPE type;
  union {
    int symbol_index;
    struct number number_value;
  } value;
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
    return NULL;
  }

  int fd = open(file_path, O_RDONLY | O_CLOEXEC);
  if (fd == -1) {
    return NULL;
  }

  off_t file_size = lseek(fd, 0, SEEK_END);
  if (file_size == -1) {
    close(fd);
    return NULL;
  }
  lseek(fd, 0, SEEK_SET);

  char *file_content = (char *)malloc(file_size + 1);
  if (file_content == NULL) {
    close(fd);
    return NULL;
  }

  ssize_t read_bytes = read(fd, file_content, file_size);
  if (read_bytes == -1) {
    free(file_content);
    close(fd);
    return NULL;
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
  while (is_letter(s->source->content[index])) {
    size += 1;
    index += 1;
  }
  unsigned int si =
      s->symbol->insert(s->symbol, s->source->content + s->index, size);
  s->index = s->index + size;
  const char *str = s->symbol->get(s->symbol, si)->str;
  if (is_keyword(str)) {
    if (strcmp(str, "true") == 0) {
      return (struct token){
          .type = TOKEN_NUM,
          .value = {.number_value = {.type = NUMBER_BOOL, .bool_value = true}}};
    }
    if (strcmp(str, "false") == 0) {
      return (struct token){.type = TOKEN_NUM,
                            .value = {.number_value = {.type = NUMBER_BOOL,
                                                       .bool_value = false}}};
    }
    if (strcmp(str, "func") == 0) {
      return (struct token){.type = TOKEN_FUNC};
    }
    if (strcmp(str, "return") == 0) {
      return (struct token){.type = TOKEN_RETURN};
    }
    if (strcmp(str, "if") == 0) {
      return (struct token){.type = TOKEN_IF};
    }
    if (strcmp(str, "else") == 0) {
      return (struct token){.type = TOKEN_ELSE};
    }
    if (strcmp(str, "while") == 0) {
      return (struct token){.type = TOKEN_WHILE};
    }

    if (strcmp(str, "include") == 0) {
      return (struct token){.type = TOKEN_INCLUDE};
    }

    if (strcmp(str, "null") == 0) {
      return (struct token){.type = TOKEN_NULL};
    }

    if (strcmp(str, "typedef") == 0) {
      return (struct token){.type = TOKEN_TYPE_DEF};
    }

    if (strcmp(str, "new") == 0) {
      return (struct token){.type = TOKEN_NEW};
    }

    if (strcmp(str, "this") == 0) {
      return (struct token){.type = TOKEN_THIS};
    }

    return (struct token){.type = TOKEN_KEYWORD, .value = {.symbol_index = si}};
  }
  return (struct token){.type = TOKEN_ID, .value = {.symbol_index = si}};
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
    return (struct token){
        .type = TOKEN_NUM,
        .value = {.number_value =
                      (struct number){.type = NUMBER_INT, .int_value = value}}};
  } else {
    double float_value = strtod(s->source->content + s->index, NULL);
    s->index += size;
    return (struct token){
        .type = TOKEN_NUM,
        .value = {.number_value = (struct number){.type = NUMBER_FLOAT,
                                                  .float_value = float_value}}};
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
  s->index = s->index + size + 1; // +1 to skip '"'
  return (struct token){.type = TOKEN_STRING, .value = {.symbol_index = si}};
}

void single_comment_consumer(struct scanner *s) {
  int first = s->index + 2;
  while (first < s->source->size) {
    if (s->source->content[first] == '\n') {
      s->index = first + 1;
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
  return t;
}

struct token next_token(struct scanner *s) {
  if (s->index >= s->source->size) {
    return (struct token){.type = TOKEN_EOF};
  }
  // skip white space
  while (is_white_space(s->source->content[s->index])) {
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
    return (struct token){.type = TOKEN_EOF};
  } else {
    return (struct token){.type = TOKEN_UNKNOWN};
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
      bool is_from_params;
      int offset;
      bool source_in_stack;
      char *func_name;
      struct syntax_expr *source;
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
    } identifier_expr;

    struct {
      struct syntax_expr *obj;
      struct Vec *props;
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
  struct syntax_expr *factory = parser_primary_call(parser);
  if (is_unary) {
    struct syntax_expr *unary =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    unary->type = EXPR_UNARY;
    unary->data.binary_expr.op = n.type;
    unary->data.binary_expr.right = factory;
    unary->data.binary_expr.left = NULL;
    factory = unary;
  }
  return factory;
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

    if (parser->sc->cur_token.type == TOKEN_ACCESS) {
      struct syntax_expr *access_expr =
          (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
      access_expr->data.access_expr.obj = id;
      access_expr->type = EXPR_ACCESS;
      access_expr->data.access_expr.props = new_vec();
      while (parser->sc->cur_token.type == TOKEN_ACCESS) {
        expected_token_type_and_run(parser->sc, TOKEN_ACCESS);
        struct token n = parser->sc->cur_token;
        expected_token_type_and_run(parser->sc, TOKEN_ID);
        access_expr->data.access_expr.props->push(
            access_expr->data.access_expr.props,
            parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index)
                ->str);
      }
      return access_expr;
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
  } else if (caller->type == EXPR_ACCESS) {
    call->data.call_expr.source_in_stack = true;
    call->data.call_expr.source = caller;
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
  do {
    if (parser->sc->cur_token.type == TOKEN_COMMA) {
      scanner_run(parser->sc);
    }
    expr->data.array_expr.elements->push(expr->data.array_expr.elements,
                                         parser_expr(parser));
  } while (parser->sc->cur_token.type == TOKEN_COMMA);
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
                             expr->data.identifier_expr.var_index + 1); // bp+d
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
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL NEW_OBJECT");
}

void expression_access_visitor(struct syntax_expr *expr) {
  assert(expr->type == EXPR_ACCESS);
  expression_visitor(expr->data.access_expr.obj);
  for (int i = 0; i < expr->data.access_expr.props->count; i++) {
    const char *prop =
        expr->data.access_expr.props->get(expr->data.access_expr.props, i);
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_S %s", prop);
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "GET");
  }
  // TODO: support pass this ptr, may be use.
  // expression_visitor(expr->data.access_expr.obj);
}

void expression_access_visitor_for_call(struct syntax_expr *expr) {
  assert(expr->type == EXPR_ACCESS);
  expression_visitor(expr->data.access_expr.obj);
  for (int i = 0; i < expr->data.access_expr.props->count; i++) {
    if (i == expr->data.access_expr.props->count - 1) {
      INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "COPY");
    }
    const char *prop =
        expr->data.access_expr.props->get(expr->data.access_expr.props, i);
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_S %s", prop);
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "GET");
  }
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
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_S \"%s\"",
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

  }

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
    expression_access_visitor_for_call(expr->data.call_expr.source);
  }

  if (expr->data.call_expr.is_from_params) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "LOAD #-%d",
                     expr->data.call_expr.offset);
  }
  for (int i = expr->data.call_expr.args->count - 1; i >= 0; i--) {
    expression_visitor(
        expr->data.call_expr.args->get(expr->data.call_expr.args, i));
  }
  if (expr->data.call_expr.source_in_stack) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL -%d",
                     expr->data.call_expr.args->count);
  } else if (expr->data.call_expr.is_from_params) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL -%d",
                     expr->data.call_expr.args->count);
  } else {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL %s",
                     expr->data.call_expr.func_name);
  }
  if (expr->data.call_expr.source_in_stack) {
    INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "FREE %d",
                     expr->data.call_expr.args->count + 2); // function and this
  } else if (expr->data.call_expr.is_from_params) {
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
  struct Vec *elements = expr->data.array_expr.elements;
  for (int i = 0; i < elements->count; i++) {
    expression_visitor(elements->get(elements, i));
  }
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "PUSH_D %d", elements->count);
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL NEW_ARRAY");
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
  INSTRUCTION_SAVE(CURRENT_FUNCTION_NAME, "CALL NEW_OBJECT");
}

typedef enum {
  VAL_INT,
  VAL_FLOAT,
  VAL_STR_INDEX,
  VAL_OBJ_PTR,
  VAL_ARR_PTR,
  VAL_BOOL,
  VAL_FUNC,
  VAL_REF,
  VAL_OFFSET,
  VAL_REGISTER,
  VAL_NULL,
} ValueType;

typedef struct {
  ValueType type;
  union {
    int i_val;
    float f_val;
    bool b_val;
    void *ptr;
  } data;
} ZValue;

typedef struct {
  int length;
  ZValue **elements;
} ZArray;

typedef struct {
  const char *key;
  ZValue *value;
} ZPair;

typedef struct {
  int count;
  ZPair *entries;
} ZObject;

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

ZValue *copy(ZValue *src);

#define Context struct zero_context

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
  // INSTRUCTION *instructions;
  struct Vec *instructions;
  bool is_builtin;
  const char *name;
  int params_count;
} ZFunction;

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

ZFunction *new_function() {
  ZFunction *func = (ZFunction *)malloc(sizeof(ZFunction));
  func->instructions = new_vec();
  func->is_builtin = false;
  func->name = NULL;
  return func;
}

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
  void *stacks[1024];
};
#define VM struct zero_vm

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
}

ZValue *get_builtin_function_value(VM *vm, const char *name) {
  ZValue *v = (ZValue *)malloc(sizeof(ZValue));
  v->type = VAL_REF;
  ZValue *b_print = (ZValue *)malloc(sizeof(ZValue));
  b_print->type = VAL_FUNC;
  ZFunction *func = new_function();
  func->is_builtin = true;
  func->name = name;
  b_print->data.ptr = func;
  v->data.ptr = b_print;
  return v;
}

void init_builtin(VM *vm) {
  map_insert(vm->symbols, "print", get_builtin_function_value(vm, "print"));
  map_insert(vm->symbols, "NEW_ARRAY",
             get_builtin_function_value(vm, "NEW_ARRAY"));
  map_insert(vm->symbols, "NEW_OBJECT",
             get_builtin_function_value(vm, "NEW_OBJECT"));
  map_insert(vm->symbols, "__print", get_builtin_function_value(vm, "__print"));
}

const Map *GET_ACTIONS() {
  if (actions == NULL) {
    init_actions();
  }
  return actions;
}

void list_inst_store(MapPair *pair, void *data) {
  VM *vm = (VM *)data;
  ZFunction *f = new_function();
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

  ZValue *fvalue = (ZValue *)malloc(sizeof(ZValue));
  fvalue->type = VAL_FUNC;
  fvalue->data.ptr = f;

  ZValue *value = (ZValue *)malloc(sizeof(ZValue));
  value->type = VAL_REF;
  value->data.ptr = fvalue;
  if (strcmp(pair->key, "__init__") == 0) {
    vm->globals->push(vm->globals, value);
  } else {
    if (strcmp(pair->key, "main") == 0) {
      vm->entry = value;
    }
    map_insert(vm->symbols, pair->key, value);
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
    ZValue *f = (ZValue *)zv->data.ptr;
    assert(f->type == VAL_FUNC);
    inst->v->data.ptr = zv->data.ptr;
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

void PUSH_INST_RUN(VM *vm, ZValue *value) { vm->stacks[++vm->sp] = value; }

void STORE_INST_RUN(VM *vm, ZValue *value) {
  ZValue *v = vm->stacks[vm->sp--];
  if (value->type == VAL_REGISTER) {
    map_insert(vm->registers, (const char *)(value->data.ptr), v);
  } else if (value->type == VAL_OFFSET) {
    /*
    When loading a reference-type data, the consumer should directly resolve or
    access the actual value being referenced, rather than the reference object
    itself.
    */
    int target_position = value->data.i_val + vm->cur_context->bp;
    // is new variable store,
    if (vm->sp < target_position) {
      vm->sp = target_position;
      if (v->type == VAL_REF) {
        ZValue *target = (ZValue *)malloc(sizeof(ZValue));
        target->type = VAL_REF;
        target->data.ptr = v->data.ptr;
        vm->stacks[target_position] = target;
      } else {
        vm->stacks[target_position] = v;
      }
    } else {
      ZValue *target = vm->stacks[target_position];
      /*
      >>> a = {"name":"mike"}
      >>> a
      {'name': 'mike'}
      >>> def run(x):
      ...     x = 3
      ...
      >>> run(a)
      >>> a
      {'name': 'mike'}
      */
      if (v->type == VAL_REF) {
        target->type = VAL_REF;
        target->data.ptr = v->data.ptr;
      } else {
        vm->stacks[target_position] = v;
      }
    }

  } else {
    if (vm->sp < value->data.i_val) {
      vm->sp = value->data.i_val;
    }

    if (v->type == VAL_REF) {
      ZValue *target = (ZValue *)malloc(sizeof(ZValue));
      target->type = VAL_REF;
      target->data.ptr = v->data.ptr;
      vm->stacks[value->data.i_val] = target;
    } else {
      vm->stacks[value->data.i_val] = v;
    }
  }
}

void LOAD_INST_RUN(VM *vm, ZValue *value) {
  // printf("LOAD---> %ld\n", value);
  /*
    VAL_OFFSET: load value by bp and offset;
    target_index = bp + offset;
  */
  if (value->type == VAL_OFFSET) {
    int offset = value->data.i_val;
    ZValue *src = vm->stacks[vm->cur_context->bp + offset];
    // if (src->type == VAL_REF) {
    vm->stacks[++vm->sp] = src;
    // } else {
    //   ZValue *zvalue = (ZValue *)malloc(sizeof(ZValue));
    //   zvalue->type = src->type;
    //   zvalue->data = src->data;
    //   vm->stacks[++vm->sp] = zvalue;
    // }
  } else if (value->type == VAL_REGISTER) {
    MapPair *rv = map_get(vm->registers, (const char *)(value->data.ptr));
    if (rv->value != NULL) {
      vm->stacks[++vm->sp] = rv->value;
      rv->value = NULL;
    } else {
      ZValue *v = (ZValue *)malloc(sizeof(ZValue));
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
    vm->stacks[++vm->sp] = vm->stacks[offset];
  } else {
    vm->stacks[++vm->sp] = value;
  }
}

ZValue *inst_run_op(TOKEN_TYPE type, ZValue *left, ZValue *right) {
  if (left->type == VAL_REF) {
    left = (ZValue *)(left->data.ptr);
  }
  if (right->type == VAL_REF) {
    right = (ZValue *)(right->data.ptr);
  }
  assert(left->type == VAL_INT || left->type == VAL_FLOAT ||
         left->type == VAL_BOOL);
  assert(right->type == VAL_INT || right->type == VAL_FLOAT ||
         right->type == VAL_BOOL);
  ZValue *result = (ZValue *)malloc(sizeof(ZValue));
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
  }

  if (right->type == VAL_INT) {
    r = right->data.i_val;
  } else if (right->type == VAL_FLOAT) {
    r = right->data.f_val;
  } else if (left->type == VAL_BOOL) {
    r = left->data.b_val ? 1 : 0;
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
  if (is_int) {
    result->type = VAL_INT;
    result->data.i_val = (int)s;
  } else {
    result->type = VAL_FLOAT;
    result->data.f_val = s;
  }
  return result;
}

void ADD_INST_RUN(VM *vm, ZValue *value) {
  ZValue *result =
      inst_run_op(TOKEN_ADD, vm->stacks[vm->sp - 1], vm->stacks[vm->sp]);
  vm->stacks[--(vm->sp)] = result;
}

void MINUS_INST_RUN(VM *vm, ZValue *value) {
  ZValue *result =
      inst_run_op(TOKEN_MINUS, vm->stacks[vm->sp - 1], vm->stacks[vm->sp]);
  vm->stacks[--(vm->sp)] = result;
}

void MULTI_INST_RUN(VM *vm, ZValue *value) {
  ZValue *result =
      inst_run_op(TOKEN_MULTI, vm->stacks[vm->sp - 1], vm->stacks[vm->sp]);
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
    return get_number_value_from_zvalue(vm, (ZValue *)v->data.ptr);
  } else if (v->type == VAL_NULL) {
    return 0;
  } else if (v->type == VAL_OBJ_PTR) {
    return (int)v->data.ptr;
  } else {
    assert(false);
  }
}

void LOGIC_INST_RUN(VM *vm, ZValue *value, INSTRUCTION_CODE code) {
  ZValue *result = (ZValue *)malloc(sizeof(ZValue));
  result->type = VAL_BOOL;
  bool v = false;
  if (code >= I_G && code <= I_NE) {
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
    default:
      assert(false);
      break;
    }
    result->data.b_val = v;
    vm->stacks[vm->sp - 1] = result;
    vm->sp -= 1;
  } else if (code == I_NOT) {
    float right = get_number_value_from_zvalue(vm, vm->stacks[vm->sp]);
    result->data.b_val = !right;
    vm->stacks[vm->sp] = result;
  } else if (code == I_NEGATE) {
    ZValue *rv = vm->stacks[vm->sp];
    float v = -1 * get_number_value_from_zvalue(vm, rv);
    ZValue *re = (ZValue *)malloc(sizeof(ZValue));
    if (rv->type == VAL_REF) {
      rv = (ZValue *)rv->data.ptr;
    }
    switch (rv->type) {
    case VAL_INT: {
      re->type = VAL_INT;
      re->data.i_val = v;
    } break;
    case VAL_FLOAT: {
      re->type = VAL_FLOAT;
      re->data.f_val = v;
    } break;
    case VAL_BOOL: {
      re->type = VAL_FLOAT;
      re->data.b_val = v != 0;
    } break;
    default:
      assert(false);
      break;
    }
    vm->stacks[vm->sp] = re;
  }
}

void ASSIGN_INST_RUN(VM *vm, ZValue *value) {
  ZValue *source = vm->stacks[vm->sp--];
  vm->stacks[vm->sp] = source;
}

void DIV_INST_RUN(VM *vm, ZValue *value) {
  ZValue *result =
      inst_run_op(TOKEN_DIV, vm->stacks[vm->sp - 1], vm->stacks[vm->sp]);
  vm->stacks[--(vm->sp)] = result;
}

void FREE_INST_RUN(VM *vm, ZValue *value) {
  assert(value->type == VAL_INT);
  int free_size = value->data.i_val;
  vm->sp -= free_size;
}

void print_ref(VM *vm, ZValue *v);

void print_data(VM *vm, ZValue *v) {
  switch (v->type) {
  case VAL_INT:
    printf("%d", v->data.i_val);
    break;
  case VAL_FLOAT:
    printf("%f", v->data.f_val);
    break;
  case VAL_STR_INDEX: {
    printf("%s", vm->cvalues->get(vm->cvalues, v->data.i_val));
  } break;
  case VAL_BOOL: {
    if (v->data.b_val) {
      printf("true");
    } else {
      printf("false");
    }
  } break;
  case VAL_REF:
    print_ref(vm, v);
    break;
  case VAL_NULL:
    printf("null");
    break;
  case VAL_FUNC: {
    ZFunction *f = (ZFunction *)v->data.ptr;
    printf("[FUNCTION %s]", f->name);
  } break;
  default:
    break;
  }
}

void print_arr(VM *vm, ZArray *arr) {
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

void print_object(VM *vm, ZValue *value) {
  ZValue *v = (ZValue *)value->data.ptr;
  ZObject *obj = (ZObject *)v->data.ptr;
  ZValue *user_print_func = NULL;
  for (int i = 0; i < obj->count; i++) {
    if (strcmp("__str__", obj->entries[i].key) == 0) {
      user_print_func = obj->entries[i].value;
      break;
    }
  }
  if (user_print_func != NULL) {
    // push this pointer: LOAD xxx
    vm->stacks[++vm->sp] = value;
    // push function object
    vm->stacks[++vm->sp] = user_print_func;
    ZValue *call_index = (ZValue *)malloc(sizeof(ZValue));
    call_index->type = VAL_INT;
    call_index->data.i_val = 0;
    CALL_INST_RUN(vm, call_index);
    vm->sp -= 2;
  } else {
    printf("{");
    for (int i = 0; i < obj->count; i++) {
      printf("\"%s\":", obj->entries[i].key);
      print_data(vm, obj->entries[i].value);
      if (i != obj->count - 1) {
        printf(", ");
      }
    }
    printf("}");
  }
}

void print_ref(VM *vm, ZValue *v) {
  assert(v->type == VAL_REF);
  ZValue *d = (ZValue *)(v->data.ptr);
  switch (d->type) {
  case VAL_ARR_PTR:
    print_arr(vm, (ZArray *)d->data.ptr);
    break;
  case VAL_OBJ_PTR:
    print_object(vm, v);
    break;
  default:
    print_data(vm, d);
    break;
  }
}

void __print(VM *vm) {
  ZValue *v = vm->stacks[vm->sp];
  print_data(vm, v);
}

void print(VM *vm) {
  __print(vm);
  printf("\n");
}

void new_array(VM *vm) {
  ZValue *count = vm->stacks[vm->sp--];
  assert(count->type == VAL_INT);
  ZValue *data = (ZValue *)malloc(sizeof(ZValue));
  data->type = VAL_ARR_PTR;
  ZArray *arr = (ZArray *)malloc(sizeof(ZArray));
  arr->length = count->data.i_val;
  arr->elements = (ZValue **)malloc(sizeof(ZValue *) * arr->length);
  for (int i = 0; i < arr->length; i++) {
    ZValue *src = vm->stacks[vm->sp - arr->length + 1 + i];
    // arr->elements[i] = (ZValue *)malloc(sizeof(ZValue));
    arr->elements[i] = copy(src);
  }
  data->data.ptr = arr;
  vm->sp -= arr->length - 1;
  ZValue *i = (ZValue *)malloc(sizeof(ZValue));
  i->type = VAL_REF;
  i->data.ptr = data;
  vm->stacks[vm->sp] = i;
}

void new_object(VM *vm) {
  ZValue *count = vm->stacks[vm->sp--];
  assert(count->type == VAL_INT);
  ZValue *data = (ZValue *)malloc(sizeof(ZValue));
  data->type = VAL_OBJ_PTR;
  ZObject *arr = (ZObject *)malloc(sizeof(ZObject));
  arr->count = count->data.i_val;
  arr->entries = (ZPair *)malloc(sizeof(ZPair) * arr->count);
  for (int i = 0; i < arr->count; i++) {
    int stack_base = vm->sp - arr->count * 2;
    ZValue *key = vm->stacks[stack_base + i * 2 + 1];
    assert(key->type == VAL_STR_INDEX);
    ZValue *value = vm->stacks[stack_base + i * 2 + 2];
    arr->entries[i].key = vm->cvalues->get(vm->cvalues, key->data.i_val);
    arr->entries[i].value = copy(value);
    // arr->entries[i].value->data = value->data;
    // arr->entries[i].value->type = value->type;
  }
  data->data.ptr = arr;
  vm->sp -= arr->count * 2 - 1;
  ZValue *i = (ZValue *)malloc(sizeof(ZValue));
  i->type = VAL_REF;
  i->data.ptr = data;
  vm->stacks[vm->sp] = i;
}

void call_builtin_function(VM *vm, ZFunction *func) {
  assert(func->is_builtin);
  if (strcmp(func->name, "print") == 0) {
    print(vm);
  } else if (strcmp(func->name, "NEW_ARRAY") == 0) {
    new_array(vm);
  } else if (strcmp(func->name, "NEW_OBJECT") == 0) {
    new_object(vm);
  } else if (strcmp(func->name, "__print") == 0) {
    __print(vm);
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
}

ZValue *copy(ZValue *src) {
  switch (src->type) {
  case VAL_REF: {
    ZValue *ref = (ZValue *)malloc(sizeof(ZValue));
    ref->type = VAL_REF;
    ref->data.ptr = copy((ZValue *)(src->data.ptr));
    return ref;
  } break;
  case VAL_ARR_PTR: {
    ZValue *data = (ZValue *)malloc(sizeof(ZValue));
    data->type = VAL_ARR_PTR;
    ZArray *arr = (ZArray *)malloc(sizeof(ZArray));

    ZArray *raw = (ZArray *)(src->data.ptr);
    arr->length = raw->length;
    arr->elements = (ZValue *)malloc(sizeof(ZValue) * raw->length);
    for (int i = 0; i < arr->length; i++) {
      ZValue *src = raw->elements[i];
      arr->elements[i] = copy(src);
    }
    data->data.ptr = arr;
    return data;
  } break;
  case VAL_OBJ_PTR: {
    ZValue *data = (ZValue *)malloc(sizeof(ZValue));
    data->type = VAL_OBJ_PTR;
    ZObject *obj = (ZObject *)malloc(sizeof(ZObject));
    ZObject *raw = (ZObject *)(src->data.ptr);
    obj->count = raw->count;
    obj->entries = (ZPair *)malloc(sizeof(ZPair) * obj->count);
    for (int i = 0; i < obj->count; i++) {
      obj->entries[i].key = raw->entries[i].key;
      obj->entries[i].value = copy(raw->entries[i].value);
    }
    data->data.ptr = obj;
    return data;
  } break;
  case VAL_INT:
  case VAL_STR_INDEX:
  case VAL_BOOL:
  case VAL_FUNC:
  case VAL_FLOAT:
  case VAL_NULL:
  case VAL_OFFSET:
  case VAL_REGISTER: {
    ZValue *v = (ZValue *)malloc(sizeof(ZValue));
    v->type = src->type;
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
  assert(prop->type == VAL_STR_INDEX);
  ZValue *obj_wrapper = (ZObject *)ref->data.ptr;
  assert(obj_wrapper->type == VAL_OBJ_PTR);
  ZObject *obj = (ZObject *)obj_wrapper->data.ptr;

  const char *key = vm->cvalues->get(vm->cvalues, prop->data.i_val);
  for (int i = 0; i < obj->count; i++) {
    if (strcmp(obj->entries[i].key, key) == 0) {
      ZValue *nv = (ZValue *)malloc(sizeof(ZValue));
      nv->type = v->type;
      if (v->type == VAL_REF) {
        nv->data.ptr = v->data.ptr;
      } else {
        nv->data = v->data;
      }
      obj->entries[i].value = nv;
      vm->sp -= 3;
      return;
    }
  }
}

const char *get_type_str(int type) {
  switch (type) {
  case VAL_INT:
    return "int";
  case VAL_FLOAT:
    return "float";
  case VAL_BOOL:
    return "bool";
  case VAL_STR_INDEX:
    return "string";
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
    ZERO_ASSERT(need_check->type == VAL_STR_INDEX,
                "expected type is string, but get %s",
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
  vm->stacks[++vm->sp] = src;
}

void GET_INST_RUN(VM *vm, ZValue *value) {
  ZValue *obj_ref = vm->stacks[vm->sp - 1];
  ZValue *prop = vm->stacks[vm->sp];
  assert(obj_ref->type == VAL_REF);
  assert(prop->type = VAL_STR_INDEX);
  ZValue *obj = (ZValue *)obj_ref->data.ptr;
  assert(obj->type == VAL_OBJ_PTR);
  ZObject *raw_obj = (ZObject *)(obj->data.ptr);
  const char *key = vm->cvalues->get(vm->cvalues, prop->data.i_val);
  for (int i = 0; i < raw_obj->count; i++) {
    if (strcmp(raw_obj->entries[i].key, key) == 0) {
      vm->stacks[--(vm->sp)] = raw_obj->entries[i].value;
      return;
    }
  }
  assert(false);
}

void CALL_INST_RUN(VM *vm, ZValue *value) {
  // call from stack, eg: CALL -1
  if (value->type == VAL_INT) {
    value = vm->stacks[vm->sp + value->data.i_val];
  }
  assert(value->type == VAL_REF);
  value = (ZValue *)value->data.ptr;
  assert(value->type == VAL_FUNC);
  ZFunction *func = (ZFunction *)(value->data.ptr);
  Runnable *runnable = (Runnable *)malloc(sizeof(Runnable));
  runnable->ctx = new_context();
  runnable->func = func;
  bool vm_stack_is_empty = vm->sp < 0;
  runnable->ctx->bp = vm_stack_is_empty ? 0 : vm->sp;
  runnable->ctx->parent = vm->cur_context;
  vm->cur_context = runnable->ctx;
  if (func->is_builtin) {
    call_builtin_function(vm, func);
    vm->cur_context = runnable->ctx->parent;
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
  if (vm_stack_is_empty) {
    vm->sp = -1;
  } else {
    vm->sp = runnable->ctx->bp;
  }
}

void __INIT__RUN(VM *vm, ZValue *value) {
  assert(value->type == VAL_REF);
  value = (ZValue *)value->data.ptr;
  assert(value->type == VAL_FUNC);
  ZFunction *func = (ZFunction *)(value->data.ptr);

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
  map_insert(vm->registers, "ei", NULL);
  return vm;
}

INSTRUCTION *NEW_STORE_INSTRUCTION(VM *vm, const char *value) {
  if (value[0] == '[') {
    ZValue *zv = (ZValue *)malloc(sizeof(ZValue));
    zv->type = VAL_REGISTER;
    int full_len = strlen(value);
    assert(full_len > 2);
    int content_len = full_len - 2;
    char *name = (char *)malloc(sizeof(char) * (content_len + 1));
    memcpy(name, value + 1, content_len);
    name[content_len] = '\0';
    zv->data.ptr = name;
    return new_inst(I_STORE, zv);
  } else if (value[0] == '#') {
    int offset = (int)strtod(value + 1, NULL);
    ZValue *zv = (ZValue *)malloc(sizeof(ZValue));
    zv->type = VAL_OFFSET;
    zv->data.i_val = offset;
    return new_inst(I_STORE, zv);
  } else {
    int offset = (int)strtod(value, NULL);
    ZValue *zv = (ZValue *)malloc(sizeof(ZValue));
    zv->type = VAL_INT;
    zv->data.i_val = offset;
    return new_inst(I_STORE, zv);
  }
}

void lllllllllll(MapPair *pair, void *data) {
  printf("lllll: %s\n", pair->key);
}

INSTRUCTION *NEW_LOAD_INSTRUCTION(VM *vm, const char *value) {
  if (value[0] == '#') {
    int offset = (int)strtod(value + 1, NULL);
    ZValue *zv = (ZValue *)malloc(sizeof(ZValue));
    zv->type = VAL_OFFSET;
    zv->data.i_val = offset;
    return new_inst(I_LOAD, zv);
  } else if (value[0] == '[') {
    ZValue *zv = (ZValue *)malloc(sizeof(ZValue));
    zv->type = VAL_REGISTER;
    int full_len = strlen(value);
    assert(full_len > 2);
    int content_len = full_len - 2;
    char *name = (char *)malloc(sizeof(char) * (content_len + 1));
    memcpy(name, value + 1, content_len);
    name[content_len] = '\0';
    zv->data.ptr = name;
    return new_inst(I_LOAD, zv);
  } else if (value[0] == '.') {
    MapPair *pair = map_get(vm->symbols, value + 1);
    if (pair != NULL) {
      ZValue *f = (ZValue *)(pair->value);
      return new_inst(I_LOAD, f);
    } else {
      ZValue *vf = (ZValue *)malloc(sizeof(ZValue));
      vf->type = VAL_FUNC;
      ZValue *v = (ZValue *)malloc(sizeof(ZValue));
      v->type = VAL_REF;
      v->data.ptr = vf;
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
    ZValue *zv = (ZValue *)malloc(sizeof(ZValue));
    zv->type = VAL_INT;
    zv->data.i_val = offset;
    return new_inst(I_LOAD, zv);
  }
}

INSTRUCTION *NEW_PUSH_D_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = (ZValue *)malloc(sizeof(ZValue));
  v->type = VAL_INT;
  v->data.i_val = (int)strtod(value, NULL);
  return new_inst(I_PUSH, v);
}

INSTRUCTION *NEW_PUSH_F_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = (ZValue *)malloc(sizeof(ZValue));
  v->type = VAL_FLOAT;
  v->data.f_val = strtod(value, NULL);
  return new_inst(I_PUSH, v);
}
// do nothing now
INSTRUCTION *NEW_PUSH_B_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = (ZValue *)malloc(sizeof(ZValue));
  v->type = VAL_BOOL;
  v->data.b_val = (int)strtod(value, NULL) == 1;
  return new_inst(I_PUSH, v);
}

INSTRUCTION *NEW_PUSH_S_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = (ZValue *)malloc(sizeof(ZValue));
  v->type = VAL_STR_INDEX;
  vm->cvalues->push(vm->cvalues, value);
  v->data.i_val = vm->cvalues->count - 1;
  return new_inst(I_PUSH, v);
}

INSTRUCTION *NEW_PUSH_N_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = (ZValue *)malloc(sizeof(ZValue));
  v->type = VAL_NULL;
  return new_inst(I_PUSH, v);
}

INSTRUCTION *NEW_CALL_INSTRUCTION(VM *vm, const char *value) {
  if (value[0] != '-') {
    ZValue *vf = (ZValue *)malloc(sizeof(ZValue));
    vf->type = VAL_FUNC;
    ZValue *v = (ZValue *)malloc(sizeof(ZValue));
    v->type = VAL_REF;
    v->data.ptr = vf;
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
    ZValue *v = (ZValue *)malloc(sizeof(ZValue));
    v->type = VAL_INT;
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
  ZValue *v = (ZValue *)malloc(sizeof(ZValue));
  v->type = VAL_INT;
  v->data.i_val = (int)strtod(value, NULL);
  return new_inst(I_FREE, v);
}

INSTRUCTION *NEW_JUMP_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = (ZValue *)malloc(sizeof(ZValue));
  v->type = VAL_INT;
  v->data.i_val = (int)strtod(value, NULL);
  return new_inst(I_JUMP, v);
}

INSTRUCTION *NEW_JF_INSTRUCTION(VM *vm, const char *value) {
  ZValue *v = (ZValue *)malloc(sizeof(ZValue));
  v->type = VAL_INT;
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
  ZValue *v = (ZValue *)malloc(sizeof(ZValue));
  v->type = VAL_INT;
  v->data.i_val = (int)strtod(value, NULL);
  return new_inst(I_CHECK, v);
}
INSTRUCTION *NEW_COPY_INSTRUCTION(VM *vm, const char *value) {
  return new_inst(I_COPY, NULL);
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
