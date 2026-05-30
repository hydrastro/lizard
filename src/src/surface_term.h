#ifndef LIZARD_SURFACE_TERM_H
#define LIZARD_SURFACE_TERM_H

#include "lizard_api.h"
#include "lizard_internal.h"

/* Surface terms are parsed/expanded syntax objects at the language boundary.
 * They are deliberately untrusted: macros, readers, #lang-like front-ends, and
 * future tooling may construct them, but the kernel must never accept them
 * directly.  Today they wrap the existing AST while we migrate toward the
 * SurfaceTerm / CoreTerm / KernelTerm / Value split from the master plan. */

typedef enum {
  LIZARD_SURFACE_DATUM,
  LIZARD_SURFACE_ERROR
} lizard_surface_kind_t;

typedef struct lizard_surface_term lizard_surface_term_t;
typedef struct lizard_surface_scope_set lizard_surface_scope_set_t;
typedef struct lizard_surface_trace_event lizard_surface_trace_event_t;

typedef struct lizard_surface_list_node {
  lz_list_node_t node;
  lizard_surface_term_t *term;
} lizard_surface_list_node_t;

lizard_surface_term_t *lizard_surface_from_ast(lizard_heap_t *heap,
                                               lizard_ast_node_t *ast);
lizard_surface_term_t *lizard_surface_from_ast_span(lizard_heap_t *heap,
                                                    lizard_ast_node_t *ast,
                                                    lizard_source_span_t span);

/* Metadata propagation helpers for future macro-expansion rewrites. */
lizard_surface_term_t *lizard_surface_from_ast_like(
    lizard_heap_t *heap, lizard_ast_node_t *ast,
    const lizard_surface_term_t *origin);
lizard_surface_term_t *lizard_surface_from_quoted_datum(
    lizard_heap_t *heap, lizard_ast_node_t *datum,
    const lizard_surface_term_t *origin);
int lizard_surface_copy_scopes(lizard_heap_t *heap,
                               lizard_surface_term_t *dst,
                               const lizard_surface_term_t *src);
int lizard_surface_copy_properties(lizard_heap_t *heap,
                                   lizard_surface_term_t *dst,
                                   const lizard_surface_term_t *src);
int lizard_surface_copy_metadata(lizard_heap_t *heap,
                                 lizard_surface_term_t *dst,
                                 const lizard_surface_term_t *src,
                                 int copy_properties);
char *lizard_surface_debug_string(lizard_heap_t *heap,
                                  const lizard_surface_term_t *term);

/* Transformation tracing scaffold for future macro expansion / rewrite
 * steppers. Trace records are untrusted debugging metadata. */
int lizard_surface_trace_add(lizard_heap_t *heap,
                             lizard_surface_term_t *term,
                             const char *stage,
                             const char *detail,
                             const lizard_surface_term_t *origin);
int lizard_surface_trace_copy(lizard_heap_t *heap,
                              lizard_surface_term_t *dst,
                              const lizard_surface_term_t *src);
unsigned long lizard_surface_trace_count(const lizard_surface_term_t *term);
const char *lizard_surface_trace_latest_stage(
    const lizard_surface_term_t *term);
const char *lizard_surface_trace_latest_detail(
    const lizard_surface_term_t *term);
int lizard_surface_trace_event_at(const lizard_surface_term_t *term,
                                  unsigned long index,
                                  lizard_expansion_trace_event_t *out_event);
int lizard_surface_trace_event_string(lizard_heap_t *heap,
                                      const lizard_surface_term_t *term,
                                      unsigned long index,
                                      char *buffer,
                                      size_t buffer_size);
char *lizard_surface_trace_debug_string(lizard_heap_t *heap,
                                        const lizard_surface_term_t *term);
char *lizard_surface_origin_chain_debug_string(
    lizard_heap_t *heap, const lizard_surface_term_t *term);


lizard_surface_kind_t lizard_surface_kind(const lizard_surface_term_t *term);
lizard_ast_node_t *lizard_surface_ast(const lizard_surface_term_t *term);
int lizard_surface_span(const lizard_surface_term_t *term,
                        lizard_source_span_t *out_span);

lz_list_t *lizard_surface_list_from_ast_list(lizard_heap_t *heap,
                                             lz_list_t *ast_list);
lz_list_t *lizard_surface_parse_source(lizard_heap_t *heap,
                                       const char *source,
                                       const char *filename,
                                       lizard_diagnostic_t *diagnostic);

int lizard_surface_phase(const lizard_surface_term_t *term);
void lizard_surface_set_phase(lizard_surface_term_t *term, int phase);

/* Scope-set API.  lizard_surface_scope() is kept as a compatibility summary
 * hash for older callers; real hygiene work should use the explicit set APIs. */
lizard_surface_scope_set_t *lizard_surface_scope_set_empty(lizard_heap_t *heap);
lizard_surface_scope_set_t *lizard_surface_scope_set_add(
    lizard_heap_t *heap, const lizard_surface_scope_set_t *set,
    unsigned long scope);
lizard_surface_scope_set_t *lizard_surface_scope_set_remove(
    lizard_heap_t *heap, const lizard_surface_scope_set_t *set,
    unsigned long scope);
int lizard_surface_scope_set_contains(const lizard_surface_scope_set_t *set,
                                      unsigned long scope);
int lizard_surface_scope_set_equal(const lizard_surface_scope_set_t *a,
                                   const lizard_surface_scope_set_t *b);
unsigned long lizard_surface_scope_set_count(
    const lizard_surface_scope_set_t *set);
unsigned long lizard_surface_scope_set_summary(
    const lizard_surface_scope_set_t *set);

const lizard_surface_scope_set_t *lizard_surface_scopes(
    const lizard_surface_term_t *term);
void lizard_surface_set_scopes(lizard_surface_term_t *term,
                               lizard_surface_scope_set_t *scopes);
unsigned long lizard_surface_scope(const lizard_surface_term_t *term);
void lizard_surface_add_scope(lizard_surface_term_t *term, unsigned long scope);
void lizard_surface_remove_scope(lizard_surface_term_t *term,
                                 unsigned long scope);
int lizard_surface_has_scope(const lizard_surface_term_t *term,
                             unsigned long scope);
int lizard_surface_same_scopes(const lizard_surface_term_t *a,
                               const lizard_surface_term_t *b);
void lizard_surface_clear_scopes(lizard_surface_term_t *term);

int lizard_surface_property_set(lizard_heap_t *heap,
                                lizard_surface_term_t *term,
                                const char *key,
                                lizard_ast_node_t *value);
lizard_ast_node_t *lizard_surface_property_get(const lizard_surface_term_t *term,
                                               const char *key);
int lizard_surface_property_has(const lizard_surface_term_t *term,
                                const char *key);
int lizard_surface_property_remove(lizard_surface_term_t *term,
                                   const char *key);

#endif /* LIZARD_SURFACE_TERM_H */
