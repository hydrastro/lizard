#include "primitives.h"
#include "prims_common.h"
#include "mem.h"
#include "errors.h"

/* Split from primitives.c as part of Recoverable Core file hygiene. */

lizard_ast_node_t *lizard_primitive_datum_to_syntax(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *ctx_node, *datum, *stx;
  lizard_env_t *ctx;
  if (!lizard_prim_two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctx_node = ((lizard_ast_list_node_t *)args->head)->ast;
  datum = ((lizard_ast_list_node_t *)args->head->next)->ast;
  /* If context is a syntax object, use its context; otherwise use current env. */
  ctx = (ctx_node->type == AST_SYNTAX) ? ctx_node->data.syntax.context : env;
  stx = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  stx->type = AST_SYNTAX;
  stx->data.syntax.datum = datum;
  stx->data.syntax.context = ctx;
  stx->data.syntax.source = datum->span;
  stx->data.syntax.phase = 0;
  stx->data.syntax.scopes = NULL;
  stx->data.syntax.scope_count = 0;
  return stx;
}

lizard_ast_node_t *lizard_primitive_syntax_to_datum(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *stx;
  (void)env;
  if (!lizard_prim_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  stx = ((lizard_ast_list_node_t *)args->head)->ast;
  if (stx->type != AST_SYNTAX) {
    return stx;  /* non-syntax passes through */
  }
  return stx->data.syntax.datum;
}

lizard_ast_node_t *lizard_primitive_syntax_source(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *stx, *result;
  (void)env;
  if (!lizard_prim_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  stx = ((lizard_ast_list_node_t *)args->head)->ast;
  if (stx->type != AST_SYNTAX || stx->data.syntax.source.start_line == 0) {
    return lizard_make_nil(heap);
  }
  result = lizard_make_nil(heap);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "column", (unsigned long)stx->data.syntax.source.start_column), result);
  result = lizard_prim_cons(heap, lizard_prim_make_stat(heap, "line",   (unsigned long)stx->data.syntax.source.start_line),   result);
  return result;
}

lizard_ast_node_t *lizard_primitive_syntaxp(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  (void)env;
  if (!lizard_prim_single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_make_bool(heap,
      ((lizard_ast_list_node_t *)args->head)->ast->type == AST_SYNTAX);
}
