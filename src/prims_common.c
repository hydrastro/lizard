#include "prims_common.h"
#include "mem.h"

int lizard_prim_no_args(lz_list_t *args) {
  return args->head == args->nil;
}

int lizard_prim_single_arg(lz_list_t *args) {
  return args->head != args->nil && args->head->next == args->nil;
}

int lizard_prim_two_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next == args->nil;
}

int lizard_prim_three_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next != args->nil &&
         args->head->next->next->next == args->nil;
}

int lizard_prim_four_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next != args->nil &&
         args->head->next->next->next != args->nil &&
         args->head->next->next->next->next == args->nil;
}

lizard_ast_node_t *lizard_prim_nth_arg(lz_list_t *args, int n) {
  lz_list_node_t *cur;
  int i;
  cur = args->head;
  for (i = 0; cur != args->nil && i < n; i++) {
    cur = cur->next;
  }
  return cur == args->nil ? NULL : ((lizard_ast_list_node_t *)cur)->ast;
}

lizard_ast_node_t *lizard_prim_make_stat(lizard_heap_t *heap,
                                         const char *key,
                                         unsigned long val) {
  lizard_ast_node_t *k;
  lizard_ast_node_t *v;
  lizard_ast_node_t *p;
  k = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  v = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  k->type = AST_SYMBOL;
  k->data.variable = key;
  v->type = AST_NUMBER;
  mpz_init_set_ui(v->data.number, val);
  p->type = AST_PAIR;
  p->data.pair.car = k;
  p->data.pair.cdr = v;
  return p;
}

lizard_ast_node_t *lizard_prim_cons(lizard_heap_t *heap,
                                    lizard_ast_node_t *car,
                                    lizard_ast_node_t *cdr) {
  lizard_ast_node_t *c;
  c = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  c->type = AST_PAIR;
  c->data.pair.car = car;
  c->data.pair.cdr = cdr;
  return c;
}
