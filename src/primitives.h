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

/* Reflection / introspection. */
lizard_ast_node_t *lizard_primitive_type_of(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_env_keys(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_definedp(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_proc_arity(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap);

/* String operations. */
lizard_ast_node_t *lizard_primitive_str_length(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_str_append(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_substring(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_str_eq(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_num_to_str(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_str_to_num(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_sym_to_str(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_str_to_sym(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap);

/* Exception handling. */
lizard_ast_node_t *lizard_primitive_raise(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_try(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_error_objp(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_error_value(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_gensym(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);

/* Vectors. */
lizard_ast_node_t *lizard_primitive_make_vector(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_vector(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_vectorp(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_vec_length(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_vec_ref(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_vec_set(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_vec_to_list(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_list_to_vec(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap);

/* Hash tables. */
lizard_ast_node_t *lizard_primitive_make_hash(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_hashp(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_hash_set(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_hash_ref(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_hash_has(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_hash_size(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_hash_keys(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_hash_remove(lz_list_t *args,
                                                lizard_env_t *env,
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

/* ----- Type-theory notation primitives (no semantic checking) ----- */
lizard_ast_node_t *lizard_primitive_tt_pi(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sigma(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_pi_fresh(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_pi_freshp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sigma_fresh(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sigma_freshp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_co_pi_fresh(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_co_pi_freshp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_co_sigma_fresh(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_co_sigma_freshp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_at(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sum(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_universe(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_couniverse(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_universe_set(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_universe_setp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_couniverse_set(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_couniverse_setp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_co_max(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_co_maxp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_co_min(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_co_minp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
/* Phase H.1 — HIT primitives */
lizard_ast_node_t *lizard_primitive_tt_hit_decl(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hit_declp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hit_constructor(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hit_constructorp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hit_path(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hit_pathp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hit_ref(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hit_refp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hit_app(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hit_appp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hit_lookup(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_refl(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_inductive(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_coinductive(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_annot(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_pip(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sigmap(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_appp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sump(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_universep(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_couniversep(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_idp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_reflp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_inductivep(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_coinductivep(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_annotp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_pi_binder(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_pi_domain(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_pi_codomain(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sigma_binder(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sigma_domain(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sigma_codomain(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_app_fun(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_app_arg(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sum_left(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sum_right(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_domain(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_a(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_b(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_refl_value(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_inductive_name(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_coinductive_name(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_annot_term(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_annot_type(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_universe_level(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_couniverse_level(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_inductive_ctors(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_coinductive_dtors(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Context layer */
lizard_ast_node_t *lizard_primitive_tt_variable(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_context(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_empty_ctx(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_ctx_extend(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_ctx_lookup(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_ctx_bindings(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_ctx_length(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_substitution(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_judgment(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_uco_level(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_variablep(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_contextp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_substitutionp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_judgmentp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_var_name(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_var_type(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_subst_source(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_subst_target(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_judg_context(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_judg_term(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_judg_type(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Identity manipulation + equivalence (notation only). */
lizard_ast_node_t *lizard_primitive_tt_equiv(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_transport(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_sym(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_trans(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equivp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_transportp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_symp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_transp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_left(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_right(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_fwd(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_bwd(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_transport_path(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_transport_value(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_sym_path(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_trans_p(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_trans_q(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* TT equality engine. */
void lizard_tt_flags_init(void);
int lizard_tt_flag_get(const char *name);
void lizard_tt_flag_set(const char *name, int value);
int lizard_tt_structurally_equal(lizard_ast_node_t *a, lizard_ast_node_t *b);
lizard_ast_node_t *lizard_tt_reduce(lizard_ast_node_t *t, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_tt_reduce(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equal(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_flag_set(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_flag_get(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_flag_list(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Alpha-equivalence-aware comparison. */
int lizard_tt_alpha_equal(lizard_ast_node_t *a, lizard_ast_node_t *b);

/* TT-level lambda. */
lizard_ast_node_t *lizard_primitive_tt_lambda(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_lambdap(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_lambda_binder(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_lambda_body(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_tt_subst(lizard_ast_node_t *, const char *, lizard_ast_node_t *, lizard_heap_t *);

/* ap. */
lizard_ast_node_t *lizard_primitive_tt_ap(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_app_p_hott(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_ap_fn(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_ap_path(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* HOTT-fragment expansion. */
lizard_ast_node_t *lizard_primitive_tt_pair(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_pairp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_pair_first(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_pair_second(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_fst(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_fstp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_fst_target(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_snd(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sndp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_snd_target(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_inl(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_inlp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_inl_value(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_inr(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_inrp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_inr_value(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_case(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_casep(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_case_scrutinee(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_case_left(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_case_right(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_unit(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_unitp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_tt(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_ttp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_bot(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_botp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_j(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_jp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_j_motive(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_j_refl_case(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_j_path(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* transport with motive — for per-type-former transport rules. */
lizard_ast_node_t *lizard_primitive_tt_xport(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_xportp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_xport_motive(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_xport_path(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_xport_value(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Universe-expression layer. */
lizard_ast_node_t *lizard_primitive_tt_u_var(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_varp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_suc(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_sucp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_suc_operand(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_max(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_maxp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_max_left(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_max_right(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_min(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_minp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_min_left(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_u_min_right(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Cumulativity ordering for universes. */
int lizard_tt_universe_leq(lizard_ast_node_t *, lizard_ast_node_t *);

/* Phase L.3 — next fresh dimension. Per-process counter. */
long lizard_tt_next_fresh_dim(void);

/* Phase H.1 — HIT registry. Per-process. */
void lizard_tt_hit_register(lizard_ast_node_t *decl);
lizard_ast_node_t *lizard_tt_hit_lookup(const char *name);
lizard_ast_node_t *lizard_tt_hit_lookup_constructor_host(const char *cname);
void lizard_tt_hit_registry_reset(void);  /* mostly for tests */
long lizard_tt_hit_registry_size(void);
int lizard_tt_universe_convertible(lizard_ast_node_t *, lizard_ast_node_t *);
lizard_ast_node_t *lizard_primitive_tt_universe_leq(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_couniverse_leq(lz_list_t *, lizard_env_t *, lizard_heap_t *);
int lizard_tt_couniverse_leq(lizard_ast_node_t *, lizard_ast_node_t *);

/* Type checker. */
lizard_ast_node_t *lizard_tt_make_universe(lizard_heap_t *heap, long level);
lizard_ast_node_t *lizard_tt_infer(lizard_ast_node_t *ctx,
                                   lizard_ast_node_t *t,
                                   lizard_heap_t *heap);
int lizard_tt_check(lizard_ast_node_t *ctx,
                    lizard_ast_node_t *t,
                    lizard_ast_node_t *T,
                    lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_tt_infer(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_check(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Cubical layer — interval, paths, connection ops. */
lizard_ast_node_t *lizard_primitive_tt_interval(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_intervalp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i0(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i0p(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i1(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i1p(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i_var(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i_varp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i_and(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i_andp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i_or(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i_orp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i_neg(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_i_negp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_pathp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_domain(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_a(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_b(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_abs(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_absp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_abs_binder(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_abs_body(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_app(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_appp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_app_path(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_path_app_point(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Faces and partial elements (Turn 7). */
lizard_ast_node_t *lizard_primitive_tt_f0(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f0p(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f1(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f1p(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f_eq(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f_eqp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f_eq_left(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f_eq_right(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f_and(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f_andp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f_or(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_f_orp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_partial(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_partialp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_partial_face(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_partial_type(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sub(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_subp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sub_type(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sub_face(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sub_partial(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Face entailment (Turn 7). */
int lizard_tt_face_entails(lizard_ast_node_t *phi, lizard_ast_node_t *psi);
lizard_ast_node_t *lizard_primitive_tt_face_entails(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Kan composition (Turn 8). */
lizard_ast_node_t *lizard_primitive_tt_comp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_compp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hcomp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_hcompp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_fill(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_fillp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_comp_type_family(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_comp_face(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_comp_partial(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_comp_base(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Equivalences, Glue, ua (Turns 9 & 10). */
lizard_ast_node_t *lizard_primitive_tt_equiv_type(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_typep(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_domain(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_codomain(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_equiv(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_id_equivp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_fun(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_funp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_inv(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_equiv_invp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_glue(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_gluep(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_glue_base(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_glue_face(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_glue_t(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_glue_equiv(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_glue_intro(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_glue_introp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_unglue(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_unglue_p(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_ua(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_uap(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* Systems (Turn 11). */
lizard_ast_node_t *lizard_primitive_tt_system_nil(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_system_nilp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_system_cons(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_system_consp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_system_face(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_system_value(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_system_next(lz_list_t *, lizard_env_t *, lizard_heap_t *);

/* System lookup (Turn 11). */
lizard_ast_node_t *lizard_tt_system_lookup(lizard_ast_node_t *system, lizard_ast_node_t *phi);
lizard_ast_node_t *lizard_primitive_tt_system_lookup(lz_list_t *, lizard_env_t *, lizard_heap_t *);
