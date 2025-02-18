#ifndef LIZARD_PRIMITIVES_H
#define LIZARD_PRIMITIVES_H

#include "lizard.h"

lizard_ast_node_t *lizard_primitive_plus(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_minus(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_multiply(list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_divide(list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_equal(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_pow(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_lt(list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_le(list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_gt(list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_ge(list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_mod(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_cons(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_car(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_cdr(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_list(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_tokens(list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_ast(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_eval(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_unquote(list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
#endif /* LIZARD_PRIMITIVES_H */