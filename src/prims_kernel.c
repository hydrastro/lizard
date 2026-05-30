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
#include "elaborator.h"
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

/* (check-holes term type) — elaborate `term` against `type`.  Holes are
 * written ?0, ?1, ...  For each hole the elaborator reports its goal (the type
 * it must inhabit) and the local hypotheses in scope.  With no holes left, the
 * finished term is confirmed by the kernel. */
lizard_ast_node_t *lizard_primitive_check_holes(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *term_expr, *type_expr;
  kterm_t *term, *type;
  kctx_t *ctx;
  elab_state_t st;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  term_expr = ((lizard_ast_list_node_t *)args->head)->ast;
  type_expr = ((lizard_ast_list_node_t *)args->head->next)->ast;
  term = lizard_kernel_sexp_to_kterm(heap, term_expr);
  type = lizard_kernel_sexp_to_kterm(heap, type_expr);
  if (term == NULL || type == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);

  if (!elab_run(heap, ctx, term, type, &st)) {
    printf("Elaboration failed: the term does not fit the goal type.\n");
    return lizard_make_bool(heap, 0);
  }
  {
    int open = 0, i;
    /* report holes solved by unification first */
    for (i = 0; i < st.n_holes; i++) {
      meta_entry_t *e = meta_lookup(st.mctx, st.holes[i].id);
      if (e != NULL && e->solution != NULL) {
        printf("  ?%d := ", st.holes[i].id);
        kt_fprint(stdout, meta_zonk(heap, st.mctx, e->solution));
        printf("   (inferred)\n");
      } else {
        open++;
      }
    }
    if (open == 0) {
      kernel_result_t r = kt_check(heap, ctx, st.elaborated, type);
      if (r == KERNEL_OK) {
        printf("No open holes; kernel-verified.\n");
        return lizard_make_bool(heap, 1);
      }
      printf("No open holes, but the term failed kernel verification.\n");
      return lizard_make_bool(heap, 0);
    }
    printf("%d open hole(s):\n", open);
    for (i = 0; i < st.n_holes; i++) {
      meta_entry_t *e = meta_lookup(st.mctx, st.holes[i].id);
      kctx_entry_t *ce;
      if (e != NULL && e->solution != NULL) continue;
      printf("  ?%d : ", st.holes[i].id);
      if (st.holes[i].goal != NULL)
        kt_fprint(stdout, meta_zonk(heap, st.mctx, st.holes[i].goal));
      else
        printf("<unconstrained>");
      printf("\n");
      for (ce = st.holes[i].ctx ? st.holes[i].ctx->entries : NULL;
           ce != NULL; ce = ce->next) {
        printf("      %s : ", ce->name ? ce->name : "_");
        kt_fprint(stdout, ce->type);
        printf("\n");
      }
    }
    return lizard_make_bool(heap, 0);
  }
}

/* (elaborate term type) — elaborate `term` against `type`, inserting implicit
 * arguments and solving metavariables, then print the resulting core term and
 * confirm it with the kernel. */
lizard_ast_node_t *lizard_primitive_elaborate(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *term_expr, *type_expr;
  kterm_t *term, *type;
  kctx_t *ctx;
  elab_state_t st;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  term_expr = ((lizard_ast_list_node_t *)args->head)->ast;
  type_expr = ((lizard_ast_list_node_t *)args->head->next)->ast;
  term = lizard_kernel_sexp_to_kterm(heap, term_expr);
  type = lizard_kernel_sexp_to_kterm(heap, type_expr);
  if (term == NULL || type == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  if (!elab_run(heap, ctx, term, type, &st)) {
    printf("Elaboration failed: the term does not fit the goal type.\n");
    return lizard_make_bool(heap, 0);
  }
  printf("Elaborated: ");
  kt_fprint(stdout, st.elaborated);
  printf("\n");
  if (kt_check(heap, ctx, st.elaborated, type) == KERNEL_OK) {
    printf("Kernel-verified.\n");
    return lizard_make_bool(heap, 1);
  }
  printf("Elaborated term failed kernel verification (unsolved implicit?).\n");
  return lizard_make_bool(heap, 0);
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
  kterm_t *proof, *goal;
  kctx_t *ctx;
  const char *name = NULL;
  (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  proof = proof_qed(current_proof);
  if (proof == NULL) {
    printf("qed: %d goal(s) remaining.\n", proof_open_goals(current_proof));
    return lizard_make_bool(heap, 0);
  }
  goal = current_proof->goal_type;
  /* Trust anchor (LCF-style): tactics merely *assemble* a term; the kernel
   * independently re-checks it against the original goal.  A bug in any
   * tactic cannot produce an accepted theorem. */
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  if (kt_check(heap, ctx, proof, goal) != KERNEL_OK) {
    printf("qed: assembled proof term FAILED kernel verification.\n");
    current_proof = NULL;
    return lizard_make_bool(heap, 0);
  }
  printf("Qed! Proof term: ");
  kt_fprint(stdout, proof);
  printf("\n");
  /* (qed name) stores the verified theorem as a reusable library lemma. */
  if (args != NULL && args->head != args->nil) {
    lizard_ast_node_t *nn = ((lizard_ast_list_node_t *)args->head)->ast;
    name = (nn->type == AST_SYMBOL) ? nn->data.variable
         : (nn->type == AST_STRING) ? nn->data.string : NULL;
  }
  if (name != NULL) {
    kdef_add(heap, lizard_kernel_defs(heap), name, goal, proof);
    printf("Theorem %s stored.\n", name);
  }
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

lizard_ast_node_t *lizard_primitive_tactic_cases(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_cases(current_proof);
  if (r < 0) {
    printf("tactic-cases: innermost hypothesis is not a Bool.\n");
    return lizard_make_bool(heap, 0);
  }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_tactic_induction(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_induction(current_proof);
  if (r < 0) {
    printf("tactic-induction: innermost hypothesis is not a Nat.\n");
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
/* ----- (data '(Name (params) Sort (ctor (argtypes)) ...)) -----
 * Declare a parameterized inductive type.  Registers the type former, each
 * constructor, and an auto-generated dependent eliminator `Name-rec`.  The
 * kernel strict-positivity-checks the declaration and gives the eliminator a
 * real iota-reduction rule. */
#define DATA_MAX_P 8
#define DATA_MAX_C 16
#define DATA_MAX_A 8

static int dl_len(lizard_ast_node_t *n) {
  int c = 0;
  if (n == NULL) return 0;
  if (n->type == AST_APPLICATION) {
    lz_list_t *l = n->data.application_arguments;
    lz_list_node_t *it;
    if (l == NULL) return 0;
    for (it = l->head; it != l->nil; it = it->next) c++;
    return c;
  }
  if (n->type == AST_PAIR) {
    lizard_ast_node_t *p = n;
    while (p != NULL && p->type == AST_PAIR) { c++; p = p->data.pair.cdr; }
    return c;
  }
  return 0;
}

static lizard_ast_node_t *dl_nth(lizard_ast_node_t *n, int i) {
  if (n == NULL) return NULL;
  if (n->type == AST_APPLICATION) {
    lz_list_t *l = n->data.application_arguments;
    lz_list_node_t *it = l->head;
    while (i-- > 0 && it != l->nil) it = it->next;
    return (it == l->nil) ? NULL : ((lizard_ast_list_node_t *)it)->ast;
  }
  if (n->type == AST_PAIR) {
    lizard_ast_node_t *p = n;
    while (i-- > 0 && p != NULL && p->type == AST_PAIR) p = p->data.pair.cdr;
    return (p != NULL && p->type == AST_PAIR) ? p->data.pair.car : NULL;
  }
  return NULL;
}

static const char *dl_sym(lizard_ast_node_t *n) {
  return (n != NULL && n->type == AST_SYMBOL) ? n->data.variable : NULL;
}

lizard_ast_node_t *lizard_primitive_data(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *decl_ast, *params_ast, *sort_ast;
  kind_decl_t decl;
  const char *pnames[DATA_MAX_P];
  kterm_t *ptypes[DATA_MAX_P];
  const char *cnames[DATA_MAX_C];
  int cnargs[DATA_MAX_C];
  kterm_t **cargs[DATA_MAX_C];
  kterm_t *sort_kt, *former;
  kdef_ctx_t *dctx;
  kctx_t *cc;
  int nP, nC, i, j, k, ndecl;
  char *recname;
  size_t L;
  (void)env;
  if (args == NULL || args->head == args->nil || args->head->next != args->nil)
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  decl_ast = ((lizard_ast_list_node_t *)args->head)->ast;
  ndecl = dl_len(decl_ast);
  if (ndecl < 3) {
    printf("data: expected (Name (params) Sort ctor...)\n");
    return lizard_make_bool(heap, 0);
  }
  decl.name = dl_sym(dl_nth(decl_ast, 0));
  params_ast = dl_nth(decl_ast, 1);
  sort_ast = dl_nth(decl_ast, 2);
  if (decl.name == NULL) {
    printf("data: missing inductive name\n");
    return lizard_make_bool(heap, 0);
  }
  dctx = lizard_kernel_defs(heap);
  nP = dl_len(params_ast);
  if (nP > DATA_MAX_P) { printf("data: too many parameters\n"); return lizard_make_bool(heap, 0); }
  for (k = 0; k < nP; k++) {
    lizard_ast_node_t *b = dl_nth(params_ast, k);
    pnames[k] = dl_sym(dl_nth(b, 0));
    ptypes[k] = lizard_kernel_sexp_to_kterm_in(heap, dl_nth(b, 1), pnames, k);
    if (pnames[k] == NULL || ptypes[k] == NULL) {
      printf("data: malformed parameter %d\n", k + 1);
      return lizard_make_bool(heap, 0);
    }
  }
  sort_kt = lizard_kernel_sexp_to_kterm(heap, sort_ast);
  if (sort_kt == NULL || sort_kt->tag != KT_SORT) {
    printf("data: result kind must be a sort, e.g. (Sort 0)\n");
    return lizard_make_bool(heap, 0);
  }
  decl.n_params = nP;
  decl.param_names = pnames;
  decl.param_types = ptypes;
  decl.sort_level = sort_kt->data.sort.level;
  decl.ctor_recflags = NULL;
  former = kind_former_type(heap, &decl);
  cc = kctx_create(heap); cc->defs = dctx;
  if (kt_infer(heap, cc, former) == NULL) {
    printf("data: type former is ill-formed\n");
    return lizard_make_bool(heap, 0);
  }
  kdef_add(heap, dctx, decl.name, former, NULL);
  nC = ndecl - 3;
  if (nC < 1 || nC > DATA_MAX_C) {
    printf("data: need between 1 and %d constructors\n", DATA_MAX_C);
    return lizard_make_bool(heap, 0);
  }
  for (i = 0; i < nC; i++) {
    lizard_ast_node_t *ct = dl_nth(decl_ast, 3 + i);
    lizard_ast_node_t *al;
    cnames[i] = dl_sym(dl_nth(ct, 0));
    al = dl_nth(ct, 1);
    cnargs[i] = dl_len(al);
    if (cnames[i] == NULL || cnargs[i] > DATA_MAX_A) {
      printf("data: malformed constructor %d\n", i + 1);
      return lizard_make_bool(heap, 0);
    }
    cargs[i] = (kterm_t **)lizard_heap_alloc(sizeof(kterm_t *) * (size_t)(cnargs[i] ? cnargs[i] : 1));
    for (j = 0; j < cnargs[i]; j++) {
      cargs[i][j] = lizard_kernel_sexp_to_kterm_in(heap, dl_nth(al, j), pnames, nP);
      if (cargs[i][j] == NULL) {
        printf("data: malformed argument %d of constructor %s\n", j + 1, cnames[i]);
        return lizard_make_bool(heap, 0);
      }
    }
  }
  decl.n_ctors = nC;
  decl.ctor_names = cnames;
  decl.ctor_nargs = cnargs;
  decl.ctor_argtypes = cargs;
  L = strlen(decl.name);
  recname = (char *)lizard_heap_alloc(L + 5);
  memcpy(recname, decl.name, L);
  memcpy(recname + L, "-rec", 5);
  decl.rec_name = recname;
  if (!kind_declare(heap, dctx, &decl)) return lizard_make_bool(heap, 0);
  printf("Defined inductive %s with %d constructor(s); eliminator %s.\n",
         decl.name, nC, recname);
  return lizard_make_bool(heap, 1);
}

/* Shared core for (def …) and (theorem …): elaborate `term` against `type`
 * (inserting implicits, solving holes by unification), report any open holes
 * with their local context and goal, and on success kernel-check the
 * elaborated term and store it as a delta-transparent definition.  This is
 * term-mode proving: the proof is just a term; tactics are optional. */
static lizard_ast_node_t *def_or_theorem(lz_list_t *args, lizard_heap_t *heap,
                                         int thm) {
  lizard_ast_node_t *name_node, *type_expr, *term_expr;
  const char *name;
  kterm_t *type, *term;
  kctx_t *ctx;
  elab_state_t st;
  int open = 0, i;
  if (!three_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  type_expr = ((lizard_ast_list_node_t *)args->head->next)->ast;
  term_expr = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  name = (name_node->type == AST_SYMBOL) ? name_node->data.variable
       : (name_node->type == AST_STRING) ? name_node->data.string : "?";
  type = lizard_kernel_sexp_to_kterm(heap, type_expr);
  term = lizard_kernel_sexp_to_kterm(heap, term_expr);
  if (type == NULL || term == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  ctx->defs = lizard_kernel_defs(heap);
  if (!elab_run(heap, ctx, term, type, &st)) {
    printf("%s %s: the %s does not have the stated type.\n",
           thm ? "theorem" : "def", name, thm ? "proof" : "term");
    return lizard_make_bool(heap, 0);
  }
  for (i = 0; i < st.n_holes; i++) {
    meta_entry_t *e = meta_lookup(st.mctx, st.holes[i].id);
    if (e == NULL || e->solution == NULL) open++;
  }
  if (open > 0) {
    printf("%s %s: %d open goal(s):\n", thm ? "theorem" : "def", name, open);
    for (i = 0; i < st.n_holes; i++) {
      meta_entry_t *e = meta_lookup(st.mctx, st.holes[i].id);
      kctx_entry_t *ce;
      if (e != NULL && e->solution != NULL) continue;
      printf("  ?%d : ", st.holes[i].id);
      if (st.holes[i].goal != NULL)
        kt_fprint(stdout, meta_zonk(heap, st.mctx, st.holes[i].goal));
      else
        printf("<unconstrained>");
      printf("\n");
      for (ce = st.holes[i].ctx ? st.holes[i].ctx->entries : NULL;
           ce != NULL; ce = ce->next) {
        printf("      %s : ", ce->name ? ce->name : "_");
        kt_fprint(stdout, ce->type);
        printf("\n");
      }
    }
    return lizard_make_bool(heap, 0);
  }
  if (kt_check(heap, ctx, st.elaborated, type) != KERNEL_OK) {
    printf("%s %s: elaborated %s failed kernel verification.\n",
           thm ? "theorem" : "def", name, thm ? "proof" : "term");
    return lizard_make_bool(heap, 0);
  }
  kdef_add(heap, lizard_kernel_defs(heap), name, type, st.elaborated);
  if (thm) {
    printf("Theorem %s : ", name);
    kt_fprint(stdout, type);
    printf("\n  proved (kernel-checked).\n");
  } else {
    printf("Defined %s : ", name);
    kt_fprint(stdout, type);
    printf("\n");
  }
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_def(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  (void)env;
  return def_or_theorem(args, heap, 0);
}

lizard_ast_node_t *lizard_primitive_theorem(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  (void)env;
  return def_or_theorem(args, heap, 1);
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
