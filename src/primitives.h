#ifndef LIZARD_PRIMITIVES_H
#define LIZARD_PRIMITIVES_H

#include "lizard.h"

lizard_ast_node_t *lizard_primitive_plus(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_minus(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_multiply(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_divide(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_equal(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_pow(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_lt(lz_list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_le(lz_list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_gt(lz_list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_ge(lz_list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_mod(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_cons(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_car(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_cdr(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_list(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_tokens(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_ast(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_eval(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_unquote(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_callcc(
    lz_list_t *args, lizard_env_t *env, lizard_heap_t *heap,
    lizard_ast_node_t *(*current_cont)(lizard_ast_node_t *, lizard_env_t *,
                                       lizard_heap_t *));
lizard_ast_node_t *lizard_identity_cont(lizard_ast_node_t *result,
                                        lizard_env_t *env, lizard_heap_t *heap);
int lizard_is_false(lizard_ast_node_t *node);
int lizard_is_true(lizard_ast_node_t *node);
lizard_ast_node_t *lizard_primitive_nullp(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_pairp(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_stringp(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_boolp(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_and(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_or(lz_list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_not(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_xor(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_nand(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_nor(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_xnor(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);

lizard_ast_node_t *lizard_primitive_display(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_write(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_newline(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_load(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);

lizard_ast_node_t *lizard_primitive_arith_shift(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_expt(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_gcd(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_lcm(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_quotient(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_remainder(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_abs(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_square(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_modexpt(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);

lizard_ast_node_t *lizard_primitive_numberp(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_symbolp(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_procedurep(lz_list_t *args, lizard_env_t *env,
                                               lizard_heap_t *heap);

/* Install every built-in primitive into `env`. Used by both the REPL
   and the test harness. */
void lizard_install_primitives(lizard_heap_t *heap, lizard_env_t *env);

int lizard_is_empty_list(lizard_ast_node_t *node);
int lizard_ast_equal(lizard_ast_node_t *a, lizard_ast_node_t *b);

lizard_ast_node_t *lizard_ast_deep_copy(lizard_ast_node_t *node,
                                        lizard_heap_t *heap);

#endif /* LIZARD_PRIMITIVES_H */
