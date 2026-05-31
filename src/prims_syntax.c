/* prims_syntax.c — extracted from primitives.c (#7 monolith split).
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
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

lizard_ast_node_t *lizard_primitive_datum_to_syntax(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *ctx_node, *datum, *stx;
  lizard_env_t *ctx;
  if (!two_args(args)) {
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
  if (!single_arg(args)) {
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
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  stx = ((lizard_ast_list_node_t *)args->head)->ast;
  if (stx->type != AST_SYNTAX || stx->data.syntax.source.start_line == 0) {
    return lizard_make_nil(heap);
  }
  result = lizard_make_nil(heap);
  result = gc_cons(heap, gc_make_stat(heap, "column", (unsigned long)stx->data.syntax.source.start_column), result);
  result = gc_cons(heap, gc_make_stat(heap, "line",   (unsigned long)stx->data.syntax.source.start_line),   result);
  return result;
}
lizard_ast_node_t *lizard_primitive_syntaxp(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_make_bool(heap,
      ((lizard_ast_list_node_t *)args->head)->ast->type == AST_SYNTAX);
}

/* (form-location form) — the source span of a datum form's head (or of the
 * form itself if it is an atom), as ((line L) (column C)), or nil if unknown.
 * Lets the macro stepper report WHERE each expansion happens. */
lizard_ast_node_t *lizard_primitive_form_location(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *form, *head, *result;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  form = ((lizard_ast_list_node_t *)args->head)->ast;
  head = (form && form->type == AST_PAIR) ? form->data.pair.car : form;
  if (!head || head->span.start_line == 0) {
    return lizard_make_nil(heap);
  }
  result = lizard_make_nil(heap);
  result = gc_cons(heap,
      gc_make_stat(heap, "column", (unsigned long)head->span.start_column),
      result);
  result = gc_cons(heap,
      gc_make_stat(heap, "line", (unsigned long)head->span.start_line),
      result);
  return result;
}

/* (read-syntax string) — tokenize and parse `string`, returning the first datum
 * WITH its source spans intact (line/column relative to the string).  This
 * gives the macro stepper real, located forms to step through. */
lizard_ast_node_t *lizard_primitive_read_syntax(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *strnode, *datum;
  lz_list_t *tokens;
  lz_list_node_t *cur;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  strnode = ((lizard_ast_list_node_t *)args->head)->ast;
  if (strnode->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_STRINGP_ARGC);
  }
  tokens = lizard_tokenize(strnode->data.string);
  if (!tokens || tokens->head == tokens->nil) {
    return lizard_make_nil(heap);
  }
  cur = tokens->head;
  datum = lizard_parse_datum(tokens, &cur, heap);
  return datum ? datum : lizard_make_nil(heap);
}
