#ifndef LIZARD_PRIMITIVES_H
#define LIZARD_PRIMITIVES_H

#include "lizard_internal.h"

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
/* Phase C: module loader. */
lizard_ast_node_t *lizard_primitive_import(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_module_loadedp(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_module_search_path(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_add_module_path(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap);
/* Phase D: GC. */
lizard_ast_node_t *lizard_primitive_gc_stats(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_gc(lz_list_t *args,
                                        lizard_env_t *env,
                                        lizard_heap_t *heap);
/* Phase E: bytecode VM. */
lizard_ast_node_t *lizard_primitive_vm_eval(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_disassemble(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_vm_time(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_time_eval(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_profile(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_error_location(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap);
/* List operations. */
lizard_ast_node_t *lizard_primitive_length(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_append(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_reverse(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_listp(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_apply(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_map(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_filter(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_for_each(lz_list_t *args, lizard_env_t *env,
                                              lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_fold_left(lz_list_t *args, lizard_env_t *env,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_assoc(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_member(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_list_ref(lz_list_t *args, lizard_env_t *env,
                                              lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_iota(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
/* Track R: syntax objects. */
lizard_ast_node_t *lizard_primitive_datum_to_syntax(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_syntax_to_datum(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_syntax_source(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_syntaxp(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* Track C: persistent vectors. */
lizard_ast_node_t *lizard_primitive_pvec(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_pvec_ref(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_pvec_set(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_pvec_push(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_pvec_count(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_pvec_to_list(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_pvecp(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* Track C: persistent hash maps. */
lizard_ast_node_t *lizard_primitive_phash_map(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_phash_get(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* hash_set and hash_keys declared below in existing hash section */
lizard_ast_node_t *lizard_primitive_phash_set(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_phash_keys(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_phash_count(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_phash_mapp(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* Track K: kernel type checker. */
lizard_ast_node_t *lizard_primitive_kernel_check(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_kernel_infer(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* Track K: tactics. */
lizard_ast_node_t *lizard_primitive_begin_proof(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_proof_state(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_tactic_intro(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_tactic_exact(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_tactic_refl(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_qed(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* Track C.4: atoms. */
lizard_ast_node_t *lizard_primitive_atom(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_deref(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_swap(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_reset(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_atomp(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* Exceptions — raise already declared below. */
lizard_ast_node_t *lizard_primitive_guard(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* String operations. */
lizard_ast_node_t *lizard_primitive_string_ref(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_string_contains(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_string_upcase(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_string_downcase(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_string_split(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_string_join(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* Lazy evaluation — delay/force declared in lizard_internal.h */
lizard_ast_node_t *lizard_primitive_promisep(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* Kernel reduction. */
lizard_ast_node_t *lizard_primitive_kernel_reduce(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_kernel_equalp(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_tactic_assumption(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_phash_values(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_phash_entries(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
/* Metavariables. */
lizard_ast_node_t *lizard_primitive_kernel_hole(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_kernel_solve(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_kernel_zonk(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_kernel_meta_state(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_kernel_unify(lz_list_t *args,
    lizard_env_t *env, lizard_heap_t *heap);

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

/* Optional proof-theory scaffolds. */
lizard_ast_node_t *lizard_primitive_tt_s1(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_s1p(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_s1_base(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_s1_basep(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_s1_loop(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_s1_loopp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_truncp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_level(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_type(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_intro(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_introp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_value(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_elim(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_elimp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_elim_motive(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_elim_handler(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_elim_value(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_trunc_elim_prop(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_extension(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_extensionp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_extension_name(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_extension_args(lz_list_t *, lizard_env_t *, lizard_heap_t *);

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
/* Phase M.5.1 — Box and Diamond modal type constructors. */
lizard_ast_node_t *lizard_primitive_tt_box(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_boxp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_arg(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamondp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_arg(lz_list_t *, lizard_env_t *, lizard_heap_t *);
/* Phase M.5.2 — Box intro and elim. */
lizard_ast_node_t *lizard_primitive_tt_box_intro(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_introp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_intro_body(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_elim(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_elimp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_elim_binder(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_elim_scrutinee(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_elim_body(lz_list_t *, lizard_env_t *, lizard_heap_t *);
/* Phase M.5.5 — Diamond intro and elim. */
lizard_ast_node_t *lizard_primitive_tt_diamond_intro(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_introp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_intro_body(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_elim(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_elimp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_elim_binder(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_elim_scrutinee(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_elim_body(lz_list_t *, lizard_env_t *, lizard_heap_t *);
/* Phase M.5.6 — K-axiom application. */
lizard_ast_node_t *lizard_primitive_tt_box_app(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_appp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_app_fun(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_box_app_arg(lz_list_t *, lizard_env_t *, lizard_heap_t *);
/* Phase M.5.8 — Diamond Kleisli composition. */
lizard_ast_node_t *lizard_primitive_tt_diamond_bind(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_bindp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_bind_fun(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_bind_arg(lz_list_t *, lizard_env_t *, lizard_heap_t *);
/* Phase M.5.9 — symmetric S5 forms. */
lizard_ast_node_t *lizard_primitive_tt_diamond_intro_sym(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_intro_symp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_diamond_intro_sym_body(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_poss_coerce(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_poss_coercep(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_poss_coerce_body(lz_list_t *, lizard_env_t *, lizard_heap_t *);
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

/* Phase M.1 — logic-rule configuration registry. Per-process.
 *
 * A "rule" is a named toggle that future kernel-typing code can
 * consult to decide whether to apply a given inference rule. M.1
 * lands the infrastructure only — no rule is yet pre-registered
 * and no type-checking site consults the registry. */
void lizard_logic_rule_register(const char *name, int default_enabled);
int  lizard_logic_rule_enable(const char *name);
int  lizard_logic_rule_disable(const char *name);
int  lizard_logic_rule_enabled(const char *name);  /* -1=unknown,0=off,1=on */
long lizard_logic_config_size(void);
void lizard_logic_config_reset(void);
void *lizard_logic_snapshot(void);
void lizard_logic_restore(void *snapshot);
void lizard_logic_config_walk(int (*cb)(const char *name, int enabled,
                                         void *userdata),
                              void *userdata);
/* Public wrapper for the static contains_free_var helper — used by
 * the Phase M.2 lambda-cube classifier and similar code that needs
 * to query whether a binder appears in a body. */
int contains_free_var_public(lizard_ast_node_t *t, const char *name);

/* Phase M.3 — named logic bundles. Sugar over M.2 cube toggles. */
int lizard_logic_set_bundle(const char *name);
const char *lizard_logic_current_bundle(void);
void lizard_logic_bundles_walk(int (*cb)(const char *name, void *userdata),
                                void *userdata);
/* Lisp-facing primitive prototypes. */
lizard_ast_node_t *lizard_primitive_logic_rule_register(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_logic_rule_enable(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_logic_rule_disable(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_logic_rule_enabledp(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_logic_config(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_logic_config_size(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_logic_config_reset(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_set_logic(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_current_logic(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_list_logics(lz_list_t *, lizard_env_t *, lizard_heap_t *);
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
/* Phase M.5.2 Turn 2 — modal kernel API with two contexts.
 *
 * The valid context (Δ) holds hypotheses introduced by `unbox` —
 * "necessarily true" hypotheses available even inside a future
 * `box`. The truth context (Γ) holds ordinary hypotheses introduced
 * by Pi / Sigma / let / case — only available outside `box`.
 *
 * The S4 box-intro rule:  Δ; · ⊢ e : T  ⊢  Δ; Γ ⊢ box e : Box T
 * drops Γ when checking the box body — only Δ is visible inside.
 *
 * The S4 unbox rule:      Δ, x:T; Γ ⊢ body : U  given Δ; Γ ⊢ b : Box T
 * extends Δ (not Γ) with the unboxed variable.
 *
 * Backward-compat: the existing single-context functions are now
 * wrappers that pass nil for the valid context. */
lizard_ast_node_t *lizard_tt_infer2(lizard_ast_node_t *valid_ctx,
                                    lizard_ast_node_t *truth_ctx,
                                    lizard_ast_node_t *t,
                                    lizard_heap_t *heap);
int lizard_tt_check2(lizard_ast_node_t *valid_ctx,
                     lizard_ast_node_t *truth_ctx,
                     lizard_ast_node_t *t,
                     lizard_ast_node_t *T,
                     lizard_heap_t *heap);

/* Phase M.5.9 Turn 2a — judgment-kind tracking infrastructure.
 *
 * In symmetric S5 (Pfenning-Davies), each typing judgment carries a
 * KIND in addition to a type:
 *
 *   LIZARD_KIND_TRUE   — A true       (in the current world)
 *   LIZARD_KIND_VALID  — A valid      (under all worlds)
 *   LIZARD_KIND_POSS   — A poss       (in some world)
 *
 * Existing code that doesn't care about kinds keeps calling infer2 /
 * infer3, which under the hood produce LIZARD_KIND_TRUE — matching
 * the implicit assumption of all pre-M.5.9 code.
 *
 * Turn 2a adds the infrastructure: every existing rule produces
 * TRUE-kind. Turn 2b will introduce rules that produce other kinds
 * (notably dia → TRUE from POSS body, poss-coerce → POSS from TRUE
 * body) and consumers that check the kind.
 *
 * The `out_kind` parameter is OPTIONAL: pass NULL if you don't care.
 * When non-NULL, the function writes the judgment kind of the
 * inferred result. */
typedef enum {
  LIZARD_KIND_TRUE  = 0,
  LIZARD_KIND_VALID = 1,
  LIZARD_KIND_POSS  = 2
} lizard_judgment_kind_t;

lizard_ast_node_t *lizard_tt_infer2_kind(lizard_ast_node_t *valid_ctx,
                                          lizard_ast_node_t *truth_ctx,
                                          lizard_ast_node_t *t,
                                          lizard_heap_t *heap,
                                          lizard_judgment_kind_t *out_kind);

/* Phase M.5.9 — symmetric S5 three-context kernel API.
 *
 * Pfenning-Davies symmetric modal logic uses three judgment forms:
 *   A valid    — necessarily true (under all worlds)
 *   A true     — true (in the current world)
 *   A poss     — possibly true (in some world)
 *
 * Correspondingly, three contexts:
 *   Δ (valid)  — list of valid hypotheses
 *   Γ (truth)  — list of truth hypotheses
 *   Ω (poss)   — at most ONE poss hypothesis ("current focus")
 *
 * Box and Diamond rules use these contexts symmetrically:
 *   - Box-intro:   Δ; ·; · ⊢ e : A true  →  Δ; Γ; · ⊢ box e : Box A true
 *   - Diamond-intro: Δ; Γ; · ⊢ e : A poss  →  Δ; Γ; · ⊢ dia e : Diamond A true
 *   - Box-elim:    extends Δ with the unboxed witness
 *   - Diamond-elim: places the witness in Ω as a poss hypothesis
 *
 * Phase M.5.9 Turn 1 adds the API as wrappers that pass NIL for
 * poss_ctx and forward to the existing two-context implementation.
 * The symmetric typing rules are wired in Turn 2.
 *
 * Gated on `modal-symmetric` (default off). Existing modal code
 * keeps the M.5.7/M.5.8 asymmetric behavior. */
lizard_ast_node_t *lizard_tt_infer3(lizard_ast_node_t *valid_ctx,
                                    lizard_ast_node_t *truth_ctx,
                                    lizard_ast_node_t *poss_ctx,
                                    lizard_ast_node_t *t,
                                    lizard_heap_t *heap);

/* infer3_kind: three-context, with optional judgment-kind output.
 * Turn 2a wrapper around infer2_kind (forwards, ignoring poss_ctx).
 * Turn 2b will provide a separate implementation that uses Ω. */
lizard_ast_node_t *lizard_tt_infer3_kind(lizard_ast_node_t *valid_ctx,
                                          lizard_ast_node_t *truth_ctx,
                                          lizard_ast_node_t *poss_ctx,
                                          lizard_ast_node_t *t,
                                          lizard_heap_t *heap,
                                          lizard_judgment_kind_t *out_kind);
int lizard_tt_check3(lizard_ast_node_t *valid_ctx,
                     lizard_ast_node_t *truth_ctx,
                     lizard_ast_node_t *poss_ctx,
                     lizard_ast_node_t *t,
                     lizard_ast_node_t *T,
                     lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_tt_infer(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_check(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_infer_modal(lz_list_t *, lizard_env_t *, lizard_heap_t *);
lizard_ast_node_t *lizard_primitive_tt_check_modal(lz_list_t *, lizard_env_t *, lizard_heap_t *);

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
