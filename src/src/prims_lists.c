/* prims_lists.c — extracted from primitives.c (#7 monolith split).
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

lizard_ast_node_t *lizard_primitive_list(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *first, *rest;
  lz_list_t *rest_args, *cons_args;
  lizard_ast_list_node_t *first_node, *rest_node;
  if (args->head == args->nil) {
    return lizard_make_nil(heap);
  }

  first = CAST(args->head, lizard_ast_list_node_t)->ast;
  rest_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  rest_args->head = args->head->next;
  rest_args->nil = args->nil;
  rest = lizard_primitive_list(rest_args, env, heap);

  cons_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  first_node = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  first_node->ast = first;
  list_append(cons_args, &first_node->node);
  rest_node = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  rest_node->ast = rest;
  list_append(cons_args, &rest_node->node);

  return lizard_primitive_cons(cons_args, env, heap);
}
lizard_ast_node_t *lizard_primitive_length(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *lst;
  lizard_ast_node_t *r;
  long count = 0;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lst = ((lizard_ast_list_node_t *)args->head)->ast;
  while (lst != NULL && lst->type == AST_PAIR) {
    count++;
    lst = lst->data.pair.cdr;
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, count);
  return r;
}
lizard_ast_node_t *lizard_primitive_append(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b, *result, *tail, *node;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  a = ((lizard_ast_list_node_t *)args->head)->ast;
  b = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (a == NULL || a->type == AST_NIL) return b;
  /* Copy the first list. */
  result = NULL;
  tail = NULL;
  while (a != NULL && a->type == AST_PAIR) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = a->data.pair.car;
    node->data.pair.cdr = lizard_make_nil(heap);
    if (tail != NULL) {
      tail->data.pair.cdr = node;
    } else {
      result = node;
    }
    tail = node;
    a = a->data.pair.cdr;
  }
  if (tail != NULL) {
    tail->data.pair.cdr = b;
  } else {
    result = b;
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_reverse(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *lst, *result, *node;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lst = ((lizard_ast_list_node_t *)args->head)->ast;
  result = lizard_make_nil(heap);
  while (lst != NULL && lst->type == AST_PAIR) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = lst->data.pair.car;
    node->data.pair.cdr = result;
    result = node;
    lst = lst->data.pair.cdr;
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_listp(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  while (x != NULL && x->type == AST_PAIR) {
    x = x->data.pair.cdr;
  }
  return lizard_make_bool(heap, x != NULL && x->type == AST_NIL);
}
lizard_ast_node_t *lizard_primitive_map(lz_list_t *args,
                                         lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *lst, *result, *tail;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  fn = ((lizard_ast_list_node_t *)args->head)->ast;
  lst = ((lizard_ast_list_node_t *)args->head->next)->ast;
  result = lizard_make_nil(heap);
  tail = NULL;
  while (lst != NULL && lst->type == AST_PAIR) {
    lizard_ast_node_t *val, *node;
    lz_list_t *one_arg;
    lizard_ast_list_node_t *n;
    one_arg = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n->ast = lst->data.pair.car;
    list_append(one_arg, &n->node);
    val = lizard_apply(fn, one_arg, env, heap, lizard_identity_cont);
    if (val && val->type == AST_ERROR) return val;
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = val;
    node->data.pair.cdr = lizard_make_nil(heap);
    if (tail != NULL) {
      tail->data.pair.cdr = node;
    } else {
      result = node;
    }
    tail = node;
    lst = lst->data.pair.cdr;
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_filter(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *lst, *result, *tail;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  fn = ((lizard_ast_list_node_t *)args->head)->ast;
  lst = ((lizard_ast_list_node_t *)args->head->next)->ast;
  result = lizard_make_nil(heap);
  tail = NULL;
  while (lst != NULL && lst->type == AST_PAIR) {
    lizard_ast_node_t *test, *node;
    lz_list_t *one_arg;
    lizard_ast_list_node_t *n;
    one_arg = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n->ast = lst->data.pair.car;
    list_append(one_arg, &n->node);
    test = lizard_apply(fn, one_arg, env, heap, lizard_identity_cont);
    if (test && test->type == AST_ERROR) return test;
    /* Keep if truthy (not #f and not nil). */
    if (!(test->type == AST_BOOL && !test->data.boolean) &&
        test->type != AST_NIL) {
      node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      node->type = AST_PAIR;
      node->data.pair.car = lst->data.pair.car;
      node->data.pair.cdr = lizard_make_nil(heap);
      if (tail != NULL) {
        tail->data.pair.cdr = node;
      } else {
        result = node;
      }
      tail = node;
    }
    lst = lst->data.pair.cdr;
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_for_each(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *lst, *val;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  fn = ((lizard_ast_list_node_t *)args->head)->ast;
  lst = ((lizard_ast_list_node_t *)args->head->next)->ast;
  while (lst != NULL && lst->type == AST_PAIR) {
    lz_list_t *one_arg;
    lizard_ast_list_node_t *n;
    one_arg = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n->ast = lst->data.pair.car;
    list_append(one_arg, &n->node);
    val = lizard_apply(fn, one_arg, env, heap, lizard_identity_cont);
    if (val && val->type == AST_ERROR) return val;
    lst = lst->data.pair.cdr;
  }
  return lizard_make_nil(heap);
}
lizard_ast_node_t *lizard_primitive_fold_left(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *acc, *lst;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  fn  = ((lizard_ast_list_node_t *)args->head)->ast;
  acc = ((lizard_ast_list_node_t *)args->head->next)->ast;
  lst = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  while (lst != NULL && lst->type == AST_PAIR) {
    lz_list_t *call_args;
    lizard_ast_list_node_t *n1, *n2;
    call_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    n1 = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n1->ast = acc;
    list_append(call_args, &n1->node);
    n2 = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n2->ast = lst->data.pair.car;
    list_append(call_args, &n2->node);
    acc = lizard_apply(fn, call_args, env, heap, lizard_identity_cont);
    if (acc && acc->type == AST_ERROR) return acc;
    lst = lst->data.pair.cdr;
  }
  return acc;
}
lizard_ast_node_t *lizard_primitive_assoc(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *key, *alist, *pair;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  key   = ((lizard_ast_list_node_t *)args->head)->ast;
  alist = ((lizard_ast_list_node_t *)args->head->next)->ast;
  while (alist != NULL && alist->type == AST_PAIR) {
    pair = alist->data.pair.car;
    if (pair != NULL && pair->type == AST_PAIR) {
      lizard_ast_node_t *k = pair->data.pair.car;
      /* Simple equality: same type + same value. */
      if (k->type == key->type) {
        int eq = 0;
        if (k->type == AST_NUMBER) {
          eq = (mpz_cmp(k->data.number, key->data.number) == 0);
        } else if (k->type == AST_SYMBOL) {
          eq = (strcmp(k->data.variable, key->data.variable) == 0);
        } else if (k->type == AST_STRING) {
          eq = (strcmp(k->data.string, key->data.string) == 0);
        } else if (k->type == AST_BOOL) {
          eq = (k->data.boolean == key->data.boolean);
        } else if (k->type == AST_NIL) {
          eq = 1;
        }
        if (eq) return pair;
      }
    }
    alist = alist->data.pair.cdr;
  }
  return lizard_make_bool(heap, 0);  /* #f if not found */
}
lizard_ast_node_t *lizard_primitive_member(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *key, *lst, *elem;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  key = ((lizard_ast_list_node_t *)args->head)->ast;
  lst = ((lizard_ast_list_node_t *)args->head->next)->ast;
  while (lst != NULL && lst->type == AST_PAIR) {
    elem = lst->data.pair.car;
    if (elem->type == key->type) {
      int eq = 0;
      if (elem->type == AST_NUMBER) {
        eq = (mpz_cmp(elem->data.number, key->data.number) == 0);
      } else if (elem->type == AST_SYMBOL) {
        eq = (strcmp(elem->data.variable, key->data.variable) == 0);
      } else if (elem->type == AST_STRING) {
        eq = (strcmp(elem->data.string, key->data.string) == 0);
      } else if (elem->type == AST_BOOL) {
        eq = (elem->data.boolean == key->data.boolean);
      } else if (elem->type == AST_NIL) {
        eq = 1;
      }
      if (eq) return lst;
    }
    lst = lst->data.pair.cdr;
  }
  return lizard_make_bool(heap, 0);
}
lizard_ast_node_t *lizard_primitive_list_ref(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *lst, *idx;
  long n, i;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lst = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = mpz_get_si(idx->data.number);
  i = 0;
  while (lst != NULL && lst->type == AST_PAIR) {
    if (i == n) return lst->data.pair.car;
    i++;
    lst = lst->data.pair.cdr;
  }
  return lizard_make_error(heap, LIZARD_ERROR_USER);
}
lizard_ast_node_t *lizard_primitive_iota(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *count_node, *start_node;
  lizard_ast_node_t *result, *node;
  long count, start, i;
  (void)env;
  count_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (count_node->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  count = mpz_get_si(count_node->data.number);
  start = 0;
  if (args->head->next != args->nil) {
    start_node = ((lizard_ast_list_node_t *)args->head->next)->ast;
    if (start_node->type == AST_NUMBER) {
      start = mpz_get_si(start_node->data.number);
    }
  }
  result = lizard_make_nil(heap);
  for (i = count - 1 + start; i >= start; i--) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.cdr = result;
    node->data.pair.car = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->data.pair.car->type = AST_NUMBER;
    mpz_init_set_si(node->data.pair.car->data.number, i);
    result = node;
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_list_to_string(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *lst, *result;
  size_t len;
  char *buf, *p;
  lizard_ast_node_t *cur;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  lst = ((lizard_ast_list_node_t *)args->head)->ast;
  len = 0;
  for (cur = lst; cur != NULL && cur->type == AST_PAIR; cur = cur->data.pair.cdr) {
    if (cur->data.pair.car->type == AST_STRING)
      len += strlen(cur->data.pair.car->data.string);
  }
  buf = (char *)lizard_heap_alloc(len + 1);
  p = buf;
  for (cur = lst; cur != NULL && cur->type == AST_PAIR; cur = cur->data.pair.cdr) {
    if (cur->data.pair.car->type == AST_STRING) {
      size_t slen = strlen(cur->data.pair.car->data.string);
      memcpy(p, cur->data.pair.car->data.string, slen);
      p += slen;
    }
  }
  *p = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}
lizard_ast_node_t *lizard_primitive_list_to_vec(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *xs, *cur, *v;
  size_t n, i;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  xs = ((lizard_ast_list_node_t *)args->head)->ast;
  /* count length */
  n = 0;
  cur = xs;
  while (cur && cur->type == AST_PAIR) {
    n++;
    cur = cur->data.pair.cdr;
  }
  if (cur && cur->type != AST_NIL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  v = make_vector(heap, n, lizard_make_nil(heap));
  cur = xs;
  for (i = 0; i < n; i++) {
    v->data.vector.elements[i] = cur->data.pair.car;
    cur = cur->data.pair.cdr;
  }
  return v;
}
