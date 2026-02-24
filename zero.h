#ifndef __ZERO_PUBLIC_H__
#define __ZERO_PUBLIC_H__

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vec.h"

typedef enum {
  TOKEN_UNKNOWN = -1,
  TOKEN_STRING,
  TOKEN_NUM,
  TOKEN_ADD,
  TOKEN_MINUS,
  TOKEN_DIV,
  TOKEN_MULTI,
  TOKEN_EQUAL,
  TOKEN_ID,
  TOKEN_KEYWORD,
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
  TOKEN_EOF
} TOKEN_TYPE;

typedef enum {
  NUMBER_INT,
  NUMBER_FLOAT,
} NUMBER_Type;

struct number {
  NUMBER_Type type;
  union {
    int int_value;
    double float_value;
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

unsigned int str_store_insert_impl(struct str_store *self, const char *src,
                                   unsigned int size);

struct str_store *str_store_init();

void free_str_store(struct str_store *);

struct str_item *free_str_item(struct str_item *item);

void token_print(struct token, struct scanner *sc);

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
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_opcode(char c) {
  return c == '"' || c == ';' || c == ':' || c == '+' || c == '-' || c == '*' ||
         c == '/' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' ||
         c == '}' || c == '=' || c == ',' || c == '.';
}

bool is_keyword(const char *str) { return strcmp(str, "var") == 0; }

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
  if (is_keyword(s->symbol->get(s->symbol, si)->str)) {
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

struct token scanner_opcode(struct scanner *s) {
  struct token t = {.type = TOKEN_UNKNOWN};
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
  case '/':
    t = (struct token){.type = TOKEN_DIV};
    break;
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
  case '=':
    t = (struct token){.type = TOKEN_EQUAL};
    break;
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
  case TOKEN_EQUAL:
    printf("TOKEN_EQUAL:=\n");
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
#include <assert.h>

void expected_token_type_and_run(struct scanner *sc, TOKEN_TYPE type) {
  assert(sc->cur_token.type == type);
  scanner_run(sc);
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
    EXPR_LITERAL,
    EXPR_CALL,
    EXPR_ARRAY,
    EXPR_OBJECT,
    EXPR_ID
  } type;

  union {
    struct {
      struct syntax_expr *left;
      uint8_t op;
      struct syntax_expr *right;
    } binary_expr;

    struct {
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
      char *name;
    } identifier_expr;

    struct {
      enum { LIT_INT, LIT_FLOAT, LIT_STR, LIT_BOOL } kind;

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
  enum { STMT_VAR_DECL, STMT_EXPR } type;

  union {
    struct {
      char *name;
      struct syntax_expr *initializer;
    } var_stmt;

    struct {
      struct syntax_expr *expr;
    } expr_stmt;
  } data;
};

struct syntax_program {
  // struct syntax_statement **statements;
  // unsigned int count;
  // unsigned int capacity;
  struct Vec *statements;
};

struct Parser {
  struct scanner *sc;
};

struct syntax_statement *parser_var_decl_stmt(struct Parser *parser);
struct syntax_statement *parser_expr_stmt(struct Parser *parser);
struct syntax_statement *parser_statement(struct Parser *parser);
struct syntax_expr *parser_term(struct Parser *parser);
struct syntax_expr *parser_factory(struct Parser *parser);
struct syntax_expr *parser_call(struct syntax_expr *caller,
                                struct Parser *parser);
struct syntax_expr *parser_primary(struct Parser *parser);
struct syntax_expr *parser_arraylist(struct Parser *parser);
struct syntax_expr *parser_objectlist(struct Parser *parser);
struct syntax_expr *parser_expr(struct Parser *parser);

struct syntax_program *parser_program(struct Parser *parser) {
  struct syntax_program *program =
      (struct syntax_program *)malloc(sizeof(struct syntax_program));
  program->statements = new_vec();
  while (parser->sc->cur_token.type != TOKEN_EOF) {
    struct syntax_statement *stmt = parser_statement(parser);
    program->statements->push(program->statements, stmt);
    expected_token_type_and_run(parser->sc, TOKEN_SEMICOLON);
  }
  return program;
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
  return parser_expr_stmt(parser);
}

struct syntax_statement *parser_var_decl_stmt(struct Parser *parser) {
  expected_token_type_str_value_and_run(parser->sc, TOKEN_KEYWORD, "var");
  char *id_name =
      parser->sc->symbol
          ->get(parser->sc->symbol, parser->sc->cur_token.value.symbol_index)
          ->str;
  expected_token_type_and_run(parser->sc, TOKEN_EQUAL);
  struct syntax_expr *expr = parser_expr(parser);
  struct syntax_statement *statement =
      (struct syntax_statement *)malloc(sizeof(struct syntax_statement));
  statement->type = STMT_VAR_DECL;
  statement->data.var_stmt.initializer = expr;
  statement->data.var_stmt.name = id_name;
  return statement;
}

struct syntax_statement *parser_expr_stmt(struct Parser *parser) {
  struct syntax_statement *statement =
      (struct syntax_statement *)malloc(sizeof(struct syntax_statement));
  statement->type = STMT_EXPR;
  statement->data.expr_stmt.expr = parser_expr(parser);
  return statement;
}

struct syntax_expr *parser_expr(struct Parser *parser) {
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
  struct syntax_expr *primary = parser_primary(parser);
  if (parser->sc->cur_token.type == TOKEN_LEFT_PARENT) {
    primary = parser_call(primary, parser);
  }
  return primary;
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
      number->data.literal_expr.int_val = n.value.number_value.float_value;
    }
    return number;
  }
  case TOKEN_STRING: {
    struct token n = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, TOKEN_STRING);
    struct syntax_expr *str =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    str->type = EXPR_LITERAL;
    str->data.literal_expr.kind = LIT_STR;
    str->data.literal_expr.str_val =
        parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index);
    return str;
  }
  case TOKEN_ID: {
    struct token n = parser->sc->cur_token;
    expected_token_type_and_run(parser->sc, TOKEN_ID);
    struct syntax_expr *id =
        (struct syntax_expr *)malloc(sizeof(struct syntax_expr));
    id->type = EXPR_ID;
    id->data.identifier_expr.name =
        parser->sc->symbol->get(parser->sc->symbol, n.value.symbol_index);
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
    break;
  }
}

struct syntax_expr *parser_call(struct syntax_expr *caller,
                                struct Parser *parser) {}

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
        parser->sc->symbol->get(parser->sc->symbol, key.value.symbol_index);
    expected_token_type_and_run(parser->sc, TOKEN_STRING);
    expected_token_type_and_run(parser->sc, TOKEN_COLON);
    pair->value = parser_expr(parser);
    expr->data.object_expr.pairs->push(expr->data.object_expr.pairs, pair);
  } while (parser->sc->cur_token.type == TOKEN_COMMA);
  return expr;
}

#endif
#endif
