/* src/prims_kernel_core.c -- kernel checking/reduction primitives.
 * Split from prims_kernel.c in Recoverable Core Phase 1N.
 */
#include "primitives.h"
#include "errors.h"
#include "kernel.h"
#include "kernel_sexp.h"
#include "runtime.h"
#include "mem.h"

#include <stdio.h>
#include <string.h>

#include "prims_kernel_util.h"

static int kernel_single_arg(lz_list_t *args) {
  return args->head != args->nil && args->head->next == args->nil;
}

static int kernel_two_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next == args->nil;
}

/* (kernel-infer expr) — parse S-exp as kernel term, infer type. */
lizard_ast_node_t *lizard_primitive_kernel_infer(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *expr, *result;
  kterm_t *term, *type;
  kctx_t *ctx;
  (void)env;
  if (!kernel_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = lizard_kernel_sexp_to_kterm(heap, expr);
  if (term == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  ctx = kctx_create(heap);
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

/* (kernel-check expr type-expr) — check term has the given type. */
lizard_ast_node_t *lizard_primitive_kernel_check(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *term_expr, *type_expr;
  kterm_t *term, *type;
  kctx_t *ctx;
  kernel_result_t result;
  (void)env;
  if (!kernel_two_args(args)) {
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
  result = kt_check(heap, ctx, term, type);
  return lizard_make_bool(heap, result == KERNEL_OK);
}

/* ============================================================
 * Kernel reduction + equality primitives
 * ============================================================ */

/* (kernel-reduce expr) — normalize a kernel term. */
lizard_ast_node_t *lizard_primitive_kernel_reduce(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *expr, *result;
  kterm_t *term, *reduced;
  kctx_t *ctx;
  (void)env;
  if (!kernel_single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = lizard_kernel_sexp_to_kterm(heap, expr);
  if (term == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  reduced = kt_whnf(heap, ctx, term);
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = lizard_kernel_term_to_string(heap, reduced);
  return result;
}

/* (kernel-equal? a b) — check definitional equality of two terms. */
lizard_ast_node_t *lizard_primitive_kernel_equalp(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *ea, *eb;
  kterm_t *a, *b;
  kctx_t *ctx;
  (void)env;
  if (!kernel_two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  ea = ((lizard_ast_list_node_t *)args->head)->ast;
  eb = ((lizard_ast_list_node_t *)args->head->next)->ast;
  a = lizard_kernel_sexp_to_kterm(heap, ea);
  b = lizard_kernel_sexp_to_kterm(heap, eb);
  if (a == NULL || b == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  return lizard_make_bool(heap, kt_equal(heap, ctx, a, b));
}

