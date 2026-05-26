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
lizard_ast_node_t *lizard_primitive_tt_at(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_sum(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_universe(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_couniverse(lz_list_t *, lizard_env_t *, lizard_heap_t *);
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
