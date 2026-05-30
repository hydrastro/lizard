/* tt_glue.c -- Equivalence, Glue, ua, and system constructors.
 *
 * Split from tt_equality.c as part of Recoverable Core Phase 1H.  These
 * are structural constructors only; reduction/equality rules remain in
 * tt_equality.c until the next split.
 */
#include "tt_glue.h"
#include "mem.h"

lizard_ast_node_t *lizard_tt_make_equiv_type(lizard_heap_t *heap,
                                          lizard_ast_node_t *A,
                                          lizard_ast_node_t *B) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_TYPE;
  n->data.tt_equiv_type.domain = A;
  n->data.tt_equiv_type.codomain = B;
  return n;
}
lizard_ast_node_t *lizard_tt_make_id_equiv(lizard_heap_t *heap,
                                        lizard_ast_node_t *A) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID_EQUIV;
  n->data.tt_equiv_op.operand = A;
  return n;
}
lizard_ast_node_t *lizard_tt_make_equiv_fun(lizard_heap_t *heap,
                                         lizard_ast_node_t *e) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_FUN;
  n->data.tt_equiv_op.operand = e;
  return n;
}
lizard_ast_node_t *lizard_tt_make_equiv_inv(lizard_heap_t *heap,
                                         lizard_ast_node_t *e) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_INV;
  n->data.tt_equiv_op.operand = e;
  return n;
}
lizard_ast_node_t *lizard_tt_make_glue(lizard_heap_t *heap,
                                    lizard_ast_node_t *A,
                                    lizard_ast_node_t *face,
                                    lizard_ast_node_t *T,
                                    lizard_ast_node_t *e) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_GLUE;
  n->data.tt_glue.base = A;
  n->data.tt_glue.face = face;
  n->data.tt_glue.t = T;
  n->data.tt_glue.equiv = e;
  return n;
}
lizard_ast_node_t *lizard_tt_make_glue_intro(lizard_heap_t *heap,
                                          lizard_ast_node_t *face,
                                          lizard_ast_node_t *t,
                                          lizard_ast_node_t *a) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_GLUE_INTRO;
  n->data.tt_glue_intro.face = face;
  n->data.tt_glue_intro.t = t;
  n->data.tt_glue_intro.a = a;
  return n;
}
lizard_ast_node_t *lizard_tt_make_unglue(lizard_heap_t *heap,
                                      lizard_ast_node_t *e,
                                      lizard_ast_node_t *g) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNGLUE;
  n->data.tt_unglue.equiv = e;
  n->data.tt_unglue.target = g;
  return n;
}
lizard_ast_node_t *lizard_tt_make_ua(lizard_heap_t *heap,
                                  lizard_ast_node_t *e) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UA;
  n->data.tt_ua.equiv = e;
  return n;
}

lizard_ast_node_t *lizard_tt_make_system_nil(lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SYSTEM_NIL;
  return n;
}
lizard_ast_node_t *lizard_tt_make_system_cons(lizard_heap_t *heap,
                                           lizard_ast_node_t *face,
                                           lizard_ast_node_t *value,
                                           lizard_ast_node_t *next) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SYSTEM_CONS;
  n->data.tt_system_cons.face = face;
  n->data.tt_system_cons.value = value;
  n->data.tt_system_cons.next = next;
  return n;
}

