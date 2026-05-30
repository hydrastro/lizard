/* prims_string.c — extracted from primitives.c (#7 monolith split).
 * Registration stays in primitives.c; definitions linked from here. */
#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "parser.h"
#include "printer.h"
#include "runtime.h"
#include "tokenizer.h"
#include "prims_shared.h"
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

lizard_ast_node_t *lizard_primitive_stringp(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_STRINGP_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (node_arg->ast->type == AST_STRING)
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}
lizard_ast_node_t *lizard_primitive_string_ref(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *s, *idx, *result;
  long i;
  char *buf;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s   = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (s->type != AST_STRING || idx->type != AST_NUMBER)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  i = mpz_get_si(idx->data.number);
  if (i < 0 || (size_t)i >= strlen(s->data.string))
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  buf = (char *)lizard_heap_alloc(2);
  buf[0] = s->data.string[i];
  buf[1] = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}
lizard_ast_node_t *lizard_primitive_string_contains(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *s, *sub;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s   = ((lizard_ast_list_node_t *)args->head)->ast;
  sub = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (s->type != AST_STRING || sub->type != AST_STRING)
    return lizard_make_bool(heap, 0);
  return lizard_make_bool(heap, strstr(s->data.string, sub->data.string) != NULL);
}
lizard_ast_node_t *lizard_primitive_string_upcase(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *s, *result;
  char *buf;
  size_t i, len;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  len = strlen(s->data.string);
  buf = (char *)lizard_heap_alloc(len + 1);
  for (i = 0; i < len; i++) {
    char c = s->data.string[i];
    buf[i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
  }
  buf[len] = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}
lizard_ast_node_t *lizard_primitive_string_downcase(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *s, *result;
  char *buf;
  size_t i, len;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  len = strlen(s->data.string);
  buf = (char *)lizard_heap_alloc(len + 1);
  for (i = 0; i < len; i++) {
    char c = s->data.string[i];
    buf[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
  }
  buf[len] = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}
lizard_ast_node_t *lizard_primitive_string_split(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *s_node, *d_node, *result, *node, *str;
  const char *s, *d, *start, *found;
  size_t dlen;
  char *buf;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s_node = ((lizard_ast_list_node_t *)args->head)->ast;
  d_node = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (s_node->type != AST_STRING || d_node->type != AST_STRING)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  s = s_node->data.string;
  d = d_node->data.string;
  dlen = strlen(d);
  if (dlen == 0) return lizard_make_nil(heap);
  result = lizard_make_nil(heap);
  /* Build in reverse, then reverse. */
  start = s;
  while ((found = strstr(start, d)) != NULL) {
    size_t len = (size_t)(found - start);
    buf = (char *)lizard_heap_alloc(len + 1);
    memcpy(buf, start, len);
    buf[len] = '\0';
    str = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    str->type = AST_STRING;
    str->data.string = buf;
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = str;
    node->data.pair.cdr = result;
    result = node;
    start = found + dlen;
  }
  /* Last segment. */
  {
    size_t len = strlen(start);
    buf = (char *)lizard_heap_alloc(len + 1);
    memcpy(buf, start, len);
    buf[len] = '\0';
    str = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    str->type = AST_STRING;
    str->data.string = buf;
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = str;
    node->data.pair.cdr = result;
    result = node;
  }
  /* Reverse to get correct order. */
  {
    lizard_ast_node_t *rev = lizard_make_nil(heap);
    while (result->type == AST_PAIR) {
      node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      node->type = AST_PAIR;
      node->data.pair.car = result->data.pair.car;
      node->data.pair.cdr = rev;
      rev = node;
      result = result->data.pair.cdr;
    }
    result = rev;
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_string_join(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *lst, *d_node, *result;
  const char *d;
  size_t total, dlen;
  char *buf, *p;
  int first;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  lst    = ((lizard_ast_list_node_t *)args->head)->ast;
  d_node = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (d_node->type != AST_STRING)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  d = d_node->data.string;
  dlen = strlen(d);
  /* Calculate total length. */
  total = 0;
  first = 1;
  {
    lizard_ast_node_t *cur = lst;
    while (cur != NULL && cur->type == AST_PAIR) {
      if (cur->data.pair.car->type == AST_STRING) {
        if (!first) total += dlen;
        total += strlen(cur->data.pair.car->data.string);
        first = 0;
      }
      cur = cur->data.pair.cdr;
    }
  }
  buf = (char *)lizard_heap_alloc(total + 1);
  p = buf;
  first = 1;
  {
    lizard_ast_node_t *cur = lst;
    while (cur != NULL && cur->type == AST_PAIR) {
      if (cur->data.pair.car->type == AST_STRING) {
        size_t slen;
        if (!first) { memcpy(p, d, dlen); p += dlen; }
        slen = strlen(cur->data.pair.car->data.string);
        memcpy(p, cur->data.pair.car->data.string, slen);
        p += slen;
        first = 0;
      }
      cur = cur->data.pair.cdr;
    }
  }
  *p = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}
lizard_ast_node_t *lizard_primitive_char_to_int(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *s, *r;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING || strlen(s->data.string) == 0)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, (long)(unsigned char)s->data.string[0]);
  return r;
}
lizard_ast_node_t *lizard_primitive_string_to_list(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *s, *result, *node, *ch;
  size_t len;
  int i;
  char *buf;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  len = strlen(s->data.string);
  result = lizard_make_nil(heap);
  for (i = (int)len - 1; i >= 0; i--) {
    buf = (char *)lizard_heap_alloc(2);
    buf[0] = s->data.string[i];
    buf[1] = '\0';
    ch = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    ch->type = AST_STRING;
    ch->data.string = buf;
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = ch;
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_string_reverse(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *s, *result;
  size_t len, i;
  char *buf;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  len = strlen(s->data.string);
  buf = (char *)lizard_heap_alloc(len + 1);
  for (i = 0; i < len; i++) buf[i] = s->data.string[len - 1 - i];
  buf[len] = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}
static int get_string(lz_list_node_t *node, const char **out, size_t *len) {
  lizard_ast_node_t *ast = ((lizard_ast_list_node_t *)node)->ast;
  if (ast->type != AST_STRING) return 0;
  *out = ast->data.string;
  *len = strlen(ast->data.string);
  return 1;
}
lizard_ast_node_t *lizard_primitive_str_length(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  const char *s;
  size_t len;
  lizard_ast_node_t *result;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  if (!get_string(args->head, &s, &len)) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init_set_ui(result->data.number, (unsigned long)len);
  return result;
}
lizard_ast_node_t *lizard_primitive_str_append(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lz_list_node_t *iter;
  size_t total;
  char *buf;
  size_t off;
  lizard_ast_node_t *result;
  (void)env;
  total = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    lizard_ast_node_t *a = ((lizard_ast_list_node_t *)iter)->ast;
    if (a->type != AST_STRING) {
      return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
    }
    total += strlen(a->data.string);
  }
  buf = lizard_heap_alloc(total + 1);
  off = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    const char *s = ((lizard_ast_list_node_t *)iter)->ast->data.string;
    size_t l = strlen(s);
    memcpy(buf + off, s, l);
    off += l;
  }
  buf[total] = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}
lizard_ast_node_t *lizard_primitive_substring(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  const char *s;
  size_t len;
  lizard_ast_node_t *s_ast, *start_ast, *end_ast;
  long start, end;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  s_ast = ((lizard_ast_list_node_t *)args->head)->ast;
  start_ast = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (s_ast->type != AST_STRING || start_ast->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  s = s_ast->data.string;
  len = strlen(s);
  start = mpz_get_si(start_ast->data.number);
  if (args->head->next->next != args->nil) {
    end_ast = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
    if (end_ast->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
    }
    end = mpz_get_si(end_ast->data.number);
  } else {
    end = (long)len;
  }
  if (start < 0 || end < start || (size_t)end > len) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return make_string(heap, s + start, (size_t)(end - start));
}
lizard_ast_node_t *lizard_primitive_str_eq(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  a = ((lizard_ast_list_node_t *)args->head)->ast;
  b = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (a->type != AST_STRING || b->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return lizard_make_bool(heap, strcmp(a->data.string, b->data.string) == 0);
}
lizard_ast_node_t *lizard_primitive_str_to_num(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *s, *r;
  int ok;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init(r->data.number);
  ok = (mpz_set_str(r->data.number, s->data.string, 10) == 0);
  if (!ok) {
    return lizard_make_bool(heap, false);
  }
  return r;
}
lizard_ast_node_t *lizard_primitive_str_to_sym(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *s;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return make_symbol(heap, s->data.string);
}
