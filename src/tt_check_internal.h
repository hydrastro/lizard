/* tt_check_internal.h — helpers shared between the core cubical/TT checker
 * (tt_check.c) and the extracted per-theme judgment modules
 * (tt_check_modal.c, tt_check_cubical.c).  Not part of the public API. */
#ifndef LIZARD_TT_CHECK_INTERNAL_H
#define LIZARD_TT_CHECK_INTERNAL_H

#include "lizard_internal.h"
#include "primitives.h" /* lizard_judgment_kind_t, lizard_tt_infer2 */

lizard_ast_node_t *ctx_lookup(lizard_ast_node_t *ctx, const char *name);
lizard_ast_node_t *ctx_extend(lizard_ast_node_t *ctx, const char *name,
                              lizard_ast_node_t *type, lizard_heap_t *heap);
lizard_ast_node_t *type_error(lizard_heap_t *heap, const char *msg);
int is_error(lizard_ast_node_t *t);
lizard_ast_node_t *make_path_app_local(lizard_heap_t *heap, lizard_ast_node_t *p, lizard_ast_node_t *i);

lizard_ast_node_t *infer2_modal(lizard_ast_node_t *valid_ctx,
                                lizard_ast_node_t *ctx, lizard_ast_node_t *t,
                                lizard_heap_t *heap,
                                lizard_judgment_kind_t *out_kind);
lizard_ast_node_t *infer2_cubical(lizard_ast_node_t *valid_ctx,
                                  lizard_ast_node_t *ctx, lizard_ast_node_t *t,
                                  lizard_heap_t *heap,
                                  lizard_judgment_kind_t *out_kind);

#endif
