/* prims_kernel.c — extracted from primitives.c (#7 monolith split).
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
#include "kernel.h"
#include "kernel_sexp.h"
#include "tactics.h"
#include <setjmp.h>
#include <stdint.h>
#include <string.h>


static proof_state_t **lz_cur_proof_ref(void) {
  return (proof_state_t **)&lizard_runtime_current()->kernel_proof_state;
}
#define current_proof (*lz_cur_proof_ref())

lizard_ast_node_t *lizard_primitive_kernel_infer(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *expr, *result;
  kterm_t *term, *type;
  kctx_t *ctx;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = lizard_kernel_sexp_to_kterm(heap, expr);
  if (term == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  type = kt_infer(heap, ctx, term);
  if (type == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  /* Print the inferred type to a string. */
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  {
    /* Use tmpfile since fmemopen is not C89. */
    char buf[256];
    FILE *fp = tmpfile();
    long pos;
    size_t got;
    if (fp != NULL) {
      kt_fprint(fp, type);
      fflush(fp);
      pos = ftell(fp);
      if (pos > 0 && (size_t)pos < sizeof(buf)) {
        rewind(fp);
        got = fread(buf, 1, (size_t)pos, fp);
        buf[got] = '\0';
      } else {
        buf[0] = '\0';
      }
      fclose(fp);
      {
        char *sbuf = (char *)lizard_heap_alloc(strlen(buf) + 1);
        strcpy(sbuf, buf);
        result->data.string = sbuf;
      }
    } else {
      result->data.string = "<type>";
    }
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_kernel_check(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *term_expr, *type_expr;
  kterm_t *term, *type;
  kctx_t *ctx;
  kernel_result_t result;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  term_expr = ((lizard_ast_list_node_t *)args->head)->ast;
  type_expr = ((lizard_ast_list_node_t *)args->head->next)->ast;
  term = lizard_kernel_sexp_to_kterm(heap, term_expr);
  type = lizard_kernel_sexp_to_kterm(heap, type_expr);
  if (term == NULL || type == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  result = kt_check(heap, ctx, term, type);
  return lizard_make_bool(heap, result == KERNEL_OK);
}
lizard_ast_node_t *lizard_primitive_begin_proof(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *type_expr;
  kterm_t *goal_type;
  kctx_t *ctx;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  type_expr = ((lizard_ast_list_node_t *)args->head)->ast;
  goal_type = lizard_kernel_sexp_to_kterm(heap, type_expr);
  if (goal_type == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  current_proof = proof_begin(heap, ctx, goal_type);
  printf("Proof started.\n");
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_proof_state(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  (void)args; (void)env;
  if (current_proof == NULL) { printf("No active proof.\n"); }
  else { proof_state_fprint(stdout, current_proof); }
  return lizard_make_nil(heap);
}
lizard_ast_node_t *lizard_primitive_tactic_intro(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *name_node;
  const char *name;
  int r;
  (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  name = (name_node->type == AST_STRING) ? name_node->data.string
       : (name_node->type == AST_SYMBOL) ? name_node->data.variable : "x";
  r = tactic_intro(current_proof, name);
  if (r < 0) { printf("tactic-intro: goal is not a Pi type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_tactic_exact(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  kterm_t *term;
  int r;
  (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = lizard_kernel_sexp_to_kterm(heap, expr);
  if (term == NULL) { printf("tactic-exact: cannot parse term.\n"); return lizard_make_bool(heap, 0); }
  r = tactic_exact(current_proof, term);
  if (r < 0) { printf("tactic-exact: type mismatch.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_tactic_refl(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_reflexivity(current_proof);
  if (r < 0) { printf("tactic-refl: goal is not Id a a.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_qed(lz_list_t *args,
                                         lizard_env_t *env,
                                         lizard_heap_t *heap) {
  kterm_t *proof;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  proof = proof_qed(current_proof);
  if (proof == NULL) {
    printf("qed: %d goal(s) remaining.\n", proof_open_goals(current_proof));
    return lizard_make_bool(heap, 0);
  }
  printf("Qed! Proof term: ");
  kt_fprint(stdout, proof);
  printf("\n");
  current_proof = NULL;
  return lizard_make_bool(heap, 1);
}
static const char *kt_to_string(lizard_heap_t *heap, kterm_t *t) {
  char buf[512];
  FILE *fp = tmpfile();
  long pos;
  size_t got;
  char *sbuf;
  if (fp == NULL) return "<kterm>";
  kt_fprint(fp, t);
  fflush(fp);
  pos = ftell(fp);
  if (pos <= 0 || (size_t)pos >= sizeof(buf)) {
    fclose(fp);
    return "<kterm>";
  }
  rewind(fp);
  got = fread(buf, 1, (size_t)pos, fp);
  buf[got] = '\0';
  fclose(fp);
  sbuf = (char *)lizard_heap_alloc(strlen(buf) + 1);
  strcpy(sbuf, buf);
  return sbuf;
}
lizard_ast_node_t *lizard_primitive_kernel_reduce(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *expr, *result;
  kterm_t *term, *reduced;
  kctx_t *ctx;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = lizard_kernel_sexp_to_kterm(heap, expr);
  if (term == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  reduced = kt_whnf(heap, ctx, term);
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = kt_to_string(heap, reduced);
  return result;
}
lizard_ast_node_t *lizard_primitive_kernel_equalp(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *ea, *eb;
  kterm_t *a, *b;
  kctx_t *ctx;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  ea = ((lizard_ast_list_node_t *)args->head)->ast;
  eb = ((lizard_ast_list_node_t *)args->head->next)->ast;
  a = lizard_kernel_sexp_to_kterm(heap, ea);
  b = lizard_kernel_sexp_to_kterm(heap, eb);
  if (a == NULL || b == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  return lizard_make_bool(heap, kt_equal(heap, ctx, a, b));
}
lizard_ast_node_t *lizard_primitive_tactic_assumption(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_assumption(current_proof);
  if (r < 0) {
    printf("tactic-assumption: no matching hypothesis found.\n");
    return lizard_make_bool(heap, 0);
  }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_tactic_simpl(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_simpl(current_proof);
  if (r < 0) { printf("tactic-simpl: no goal.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_tactic_split(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_split(current_proof);
  if (r < 0) { printf("tactic-split: goal is not a Sigma type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_tactic_left(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_left(current_proof);
  if (r < 0) { printf("tactic-left: goal is not a Sum type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_tactic_right(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_right(current_proof);
  if (r < 0) { printf("tactic-right: goal is not a Sum type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}
static meta_ctx_t *get_meta_ctx(lizard_heap_t *heap) {
  lizard_runtime_t *rt = lizard_runtime_current();
  if (rt->kernel_meta_ctx == NULL)
    rt->kernel_meta_ctx = meta_ctx_create(heap);
  return (meta_ctx_t *)rt->kernel_meta_ctx;
}
lizard_ast_node_t *lizard_primitive_kernel_hole(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *type_expr, *result;
  kterm_t *type, *hole;
  meta_ctx_t *mctx;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  type_expr = ((lizard_ast_list_node_t *)args->head)->ast;
  type = lizard_kernel_sexp_to_kterm(heap, type_expr);
  if (type == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  mctx = get_meta_ctx(heap);
  hole = meta_fresh(heap, mctx, type);
  printf("Created hole ?%d : ", hole->data.meta.id);
  kt_fprint(stdout, type);
  printf("\n");
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init_set_si(result->data.number, hole->data.meta.id);
  return result;
}
lizard_ast_node_t *lizard_primitive_kernel_solve(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *id_node, *term_expr;
  kterm_t *term;
  meta_ctx_t *mctx;
  int id, r;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  id_node   = ((lizard_ast_list_node_t *)args->head)->ast;
  term_expr = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (id_node->type != AST_NUMBER)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  id = (int)mpz_get_si(id_node->data.number);
  term = lizard_kernel_sexp_to_kterm(heap, term_expr);
  if (term == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  mctx = get_meta_ctx(heap);
  r = meta_solve(mctx, id, term);
  if (r < 0) {
    printf("kernel-solve: failed\n");
    return lizard_make_bool(heap, 0);
  }
  printf("Solved ?%d = ", id);
  kt_fprint(stdout, term);
  printf("\n");
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_kernel_zonk(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *expr, *result;
  kterm_t *term, *zonked;
  meta_ctx_t *mctx;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = lizard_kernel_sexp_to_kterm(heap, expr);
  if (term == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  mctx = get_meta_ctx(heap);
  zonked = meta_zonk(heap, mctx, term);
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = kt_to_string(heap, zonked);
  return result;
}
lizard_ast_node_t *lizard_primitive_kernel_meta_state(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  meta_ctx_t *mctx;
  lizard_ast_node_t *r;
  (void)args; (void)env;
  mctx = get_meta_ctx(heap);
  meta_ctx_fprint(stdout, mctx);
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, (long)meta_unsolved_count(mctx));
  return r;
}
lizard_ast_node_t *lizard_primitive_kernel_unify(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *ea, *eb;
  kterm_t *a, *b;
  kctx_t *ctx;
  meta_ctx_t *mctx;
  int result;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  ea = ((lizard_ast_list_node_t *)args->head)->ast;
  eb = ((lizard_ast_list_node_t *)args->head->next)->ast;
  a = lizard_kernel_sexp_to_kterm(heap, ea);
  b = lizard_kernel_sexp_to_kterm(heap, eb);
  if (a == NULL || b == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  mctx = get_meta_ctx(heap);
  result = kt_unify(heap, ctx, mctx, a, b);
  if (result) {
    printf("Unified successfully.\n");
    meta_ctx_fprint(stdout, mctx);
  } else {
    printf("Unification failed.\n");
  }
  return lizard_make_bool(heap, result);
}
kdef_ctx_t *lizard_kernel_defs(lizard_heap_t *heap) {
  lizard_runtime_t *rt = lizard_runtime_current();
  if (rt == NULL) return NULL;
  if (rt->kernel_def_ctx == NULL)
    rt->kernel_def_ctx = kdef_ctx_create(heap);
  return (kdef_ctx_t *)rt->kernel_def_ctx;
}
lizard_ast_node_t *lizard_primitive_kernel_define(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *name_node, *term_expr, *type_expr;
  const char *name;
  kterm_t *term, *type;
  kdef_ctx_t *dctx;
  kctx_t *ctx;
  kernel_result_t r;
  (void)env;
  if (!three_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  term_expr = ((lizard_ast_list_node_t *)args->head->next)->ast;
  type_expr = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  name = (name_node->type == AST_SYMBOL) ? name_node->data.variable
       : (name_node->type == AST_STRING) ? name_node->data.string : "?";
  term = lizard_kernel_sexp_to_kterm(heap, term_expr);
  type = lizard_kernel_sexp_to_kterm(heap, type_expr);
  if (term == NULL || type == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  /* Type-check the definition. */
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  r = kt_check(heap, ctx, term, type);
  if (r != KERNEL_OK) {
    printf("kernel-define: type error for '%s'\n", name);
    return lizard_make_bool(heap, 0);
  }
  dctx = lizard_kernel_defs(heap);
  kdef_add(heap, dctx, name, type, term);
  printf("Defined %s : ", name);
  kt_fprint(stdout, type);
  printf("\n");
  return lizard_make_bool(heap, 1);
}
/* (kernel-assume name type-expr) — postulate an axiom: a named opaque
 * constant of the given type, with no defining value.  The type must be a
 * well-formed type.  References to the name elaborate to an opaque KT_CONST
 * that the kernel types by this postulated type. */
lizard_ast_node_t *lizard_primitive_kernel_assume(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *name_node, *type_expr;
  const char *name;
  kterm_t *type, *type_type;
  kdef_ctx_t *dctx;
  kctx_t *ctx;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  type_expr = ((lizard_ast_list_node_t *)args->head->next)->ast;
  name = (name_node->type == AST_SYMBOL) ? name_node->data.variable
       : (name_node->type == AST_STRING) ? name_node->data.string : "?";
  type = lizard_kernel_sexp_to_kterm(heap, type_expr);
  if (type == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  type_type = kt_infer(heap, ctx, type);
  if (type_type == NULL || kt_whnf(heap, ctx, type_type)->tag != KT_SORT) {
    printf("kernel-assume: '%s' is not a valid type\n", name);
    return lizard_make_bool(heap, 0);
  }
  dctx = lizard_kernel_defs(heap);
  kdef_add(heap, dctx, name, type, NULL); /* NULL value = axiom */
  printf("Assumed %s : ", name);
  kt_fprint(stdout, type);
  printf("\n");
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_kernel_lookup(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *name_node, *result;
  const char *name;
  kdef_ctx_t *dctx;
  kdef_t *def;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  name = (name_node->type == AST_SYMBOL) ? name_node->data.variable
       : (name_node->type == AST_STRING) ? name_node->data.string : "?";
  dctx = lizard_kernel_defs(heap);
  def = kdef_lookup(dctx, name);
  if (def == NULL) return lizard_make_bool(heap, 0);
  /* Return the value as a string. */
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = kt_to_string(heap, def->value);
  return result;
}
