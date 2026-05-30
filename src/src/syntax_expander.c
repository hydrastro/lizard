/* src/syntax_expander.c -- SurfaceTerm adapter around the existing expander.
 *
 * Phase 2H does not rewrite the macro expander. It adds a stable adapter
 * boundary so future macro/hygiene work can preserve syntax-object metadata
 * and attach debugging traces while delegating semantics to the old expander.
 */

#include "syntax_expander.h"
#include "lizard_internal.h"

int lizard_syntax_expand_surface(lizard_expansion_context_t *context,
                                 lizard_env_t *env,
                                 const lizard_surface_term_t *input,
                                 lizard_surface_term_t **out_surface,
                                 lizard_ast_node_t **out_ast) {
  lizard_heap_t *heap;
  lizard_ast_node_t *ast;
  lizard_ast_node_t *expanded;
  lizard_surface_term_t *surface;

  if (out_surface != NULL) {
    *out_surface = NULL;
  }
  if (out_ast != NULL) {
    *out_ast = NULL;
  }
  if (context == NULL || env == NULL || input == NULL) {
    return 0;
  }

  heap = lizard_expansion_context_heap(context);
  if (heap == NULL) {
    return 0;
  }

  ast = lizard_surface_ast(input);
  if (ast == NULL) {
    return 0;
  }

  expanded = lizard_expand_macros(ast, env, heap);
  if (expanded == NULL) {
    return 0;
  }

  surface = lizard_surface_from_ast_like(heap, expanded, input);
  if (surface == NULL) {
    return 0;
  }
  if (!lizard_expansion_context_rewrite(context, surface, input,
                                        "macro-expand",
                                        "existing-expander-adapter")) {
    return 0;
  }

  if (out_surface != NULL) {
    *out_surface = surface;
  }
  if (out_ast != NULL) {
    *out_ast = expanded;
  }
  return 1;
}

int lizard_syntax_expand_ast(lizard_expansion_context_t *context,
                             lizard_env_t *env,
                             lizard_ast_node_t *input,
                             lizard_surface_term_t **out_surface,
                             lizard_ast_node_t **out_ast) {
  lizard_heap_t *heap;
  lizard_surface_term_t *surface;

  if (context == NULL || input == NULL) {
    return 0;
  }
  heap = lizard_expansion_context_heap(context);
  if (heap == NULL) {
    return 0;
  }
  surface = lizard_surface_from_ast(heap, input);
  if (surface == NULL) {
    return 0;
  }
  return lizard_syntax_expand_surface(context, env, surface, out_surface,
                                      out_ast);
}
