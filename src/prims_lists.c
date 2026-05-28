#include "primitives.h"
#include "prims_common.h"
#include "mem.h"
#include "errors.h"

/* Split from primitives.c as part of Recoverable Core file hygiene. */

lizard_ast_node_t *lizard_primitive_length(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *lst;
  lizard_ast_node_t *r;
  long count = 0;
  (void)env;
  if (!lizard_prim_single_arg(args)) {
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
  if (!lizard_prim_two_args(args)) {
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
  if (!lizard_prim_single_arg(args)) {
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
  if (!lizard_prim_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  while (x != NULL && x->type == AST_PAIR) {
    x = x->data.pair.cdr;
  }
  return lizard_make_bool(heap, x != NULL && x->type == AST_NIL);
}

lizard_ast_node_t *lizard_primitive_apply(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *arg_list, *cur;
  lz_list_t *call_args;
  (void)env;
  if (!lizard_prim_two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  fn = ((lizard_ast_list_node_t *)args->head)->ast;
  arg_list = ((lizard_ast_list_node_t *)args->head->next)->ast;

  /* Build the lz_list_t from the Scheme list. */
  call_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  cur = arg_list;
  while (cur != NULL && cur->type == AST_PAIR) {
    lizard_ast_list_node_t *n;
    n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n->ast = cur->data.pair.car;
    list_append(call_args, &n->node);
    cur = cur->data.pair.cdr;
  }

  return lizard_apply(fn, call_args, env, heap, lizard_identity_cont);
}

lizard_ast_node_t *lizard_primitive_map(lz_list_t *args,
                                         lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *lst, *result, *tail;
  (void)env;
  if (!lizard_prim_two_args(args)) {
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
  if (!lizard_prim_two_args(args)) {
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
  if (!lizard_prim_two_args(args)) {
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
  if (!lizard_prim_three_args(args)) {
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
  if (!lizard_prim_two_args(args)) {
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
  if (!lizard_prim_two_args(args)) {
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
  if (!lizard_prim_two_args(args)) {
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
