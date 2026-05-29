/* src/prims_kernel_defs.c -- named kernel definition primitives. */
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


static int kernel_three_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next != args->nil &&
         args->head->next->next->next == args->nil;
}

/* Global kernel definition context. */
static kdef_ctx_t *fallback_kdef_ctx = NULL;

static kdef_ctx_t *get_kdef_ctx(lizard_heap_t *heap) {
  if (heap != NULL && heap->runtime != NULL) {
    if (heap->runtime->kernel_kdef_ctx == NULL) {
      heap->runtime->kernel_kdef_ctx = kdef_ctx_create(heap);
    }
    return (kdef_ctx_t *)heap->runtime->kernel_kdef_ctx;
  }
  if (fallback_kdef_ctx == NULL) {
    fallback_kdef_ctx = kdef_ctx_create(heap);
  }
  return fallback_kdef_ctx;
}

/* (kernel-define name term-sexp type-sexp) — store a named definition. */
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
  if (!kernel_three_args(args))
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
  r = kt_check(heap, ctx, term, type);
  if (r != KERNEL_OK) {
    printf("kernel-define: type error for '%s'\n", name);
    return lizard_make_bool(heap, 0);
  }
  dctx = get_kdef_ctx(heap);
  kdef_add(heap, dctx, name, type, term);
  printf("Defined %s : ", name);
  kt_fprint(stdout, type);
  printf("\n");
  return lizard_make_bool(heap, 1);
}

/* (kernel-lookup name) — look up a kernel definition. */
lizard_ast_node_t *lizard_primitive_kernel_lookup(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *name_node, *result;
  const char *name;
  kdef_ctx_t *dctx;
  kdef_t *def;
  (void)env;
  if (!kernel_single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  name = (name_node->type == AST_SYMBOL) ? name_node->data.variable
       : (name_node->type == AST_STRING) ? name_node->data.string : "?";
  dctx = get_kdef_ctx(heap);
  def = kdef_lookup(dctx, name);
  if (def == NULL) return lizard_make_bool(heap, 0);
  /* Return the value as a string. */
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = lizard_kernel_term_to_string(heap, def->value);
  return result;
}

