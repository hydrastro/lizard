/* src/surface_term.c -- untrusted surface syntax boundary.
 *
 * Phase 2A scaffolding: this file introduces an explicit SurfaceTerm wrapper
 * without changing evaluator semantics.  Existing parser/macro code still
 * produces lizard_ast_node_t; the wrapper gives later reader/macro/module work
 * a stable place to attach scopes, phases, and source locations.
 */

#include "surface_term.h"
#include "mem.h"
#include "parser.h"
#include "tokenizer.h"

#include <stdio.h>
#include <string.h>

typedef struct lizard_surface_property {
  struct lizard_surface_property *next;
  const char *key;
  lizard_ast_node_t *value;
} lizard_surface_property_t;

typedef struct lizard_surface_scope_node {
  struct lizard_surface_scope_node *next;
  unsigned long scope;
} lizard_surface_scope_node_t;

struct lizard_surface_scope_set {
  lizard_surface_scope_node_t *head;
  unsigned long count;
  unsigned long summary;
};

struct lizard_surface_term {
  lizard_surface_kind_t kind;
  lizard_heap_t *heap;
  lizard_ast_node_t *ast;
  lizard_source_span_t span;
  int phase;
  lizard_surface_scope_set_t *scopes;
  lizard_surface_property_t *properties;
};

static void span_clear(lizard_source_span_t *span) {
  if (span != NULL) {
    span->filename = NULL;
    span->start_line = 0;
    span->start_column = 0;
    span->end_line = 0;
    span->end_column = 0;
    span->start_offset = 0;
    span->end_offset = 0;
  }
}

static void diagnostic_clear(lizard_diagnostic_t *diagnostic) {
  if (diagnostic != NULL) {
    diagnostic->status = LIZARD_STATUS_OK;
    span_clear(&diagnostic->span);
    diagnostic->message[0] = '\0';
  }
}

static void diagnostic_copy(lizard_diagnostic_t *dst,
                            const lizard_diagnostic_t *src) {
  if (dst != NULL && src != NULL) {
    *dst = *src;
  }
}

static void diagnostic_parse_error(lizard_diagnostic_t *diagnostic,
                                   const char *message) {
  diagnostic_clear(diagnostic);
  if (diagnostic != NULL) {
    diagnostic->status = LIZARD_STATUS_PARSE_ERROR;
    if (message != NULL) {
      strncpy(diagnostic->message, message,
              sizeof(diagnostic->message) - 1U);
      diagnostic->message[sizeof(diagnostic->message) - 1U] = '\0';
    }
  }
}

static const char *surface_copy_key(lizard_heap_t *heap, const char *key) {
  char *copy;
  size_t len;
  if (heap == NULL || key == NULL) {
    return NULL;
  }
  len = strlen(key) + 1U;
  copy = (char *)lizard_heap_alloc(len);
  memcpy(copy, key, len);
  return copy;
}


static unsigned long scope_hash(unsigned long scope) {
  /* Compatibility summary: old lizard_surface_scope() XORed raw markers. */
  return scope;
}

static lizard_surface_scope_set_t *scope_set_alloc(lizard_heap_t *heap) {
  lizard_surface_scope_set_t *set;
  if (heap == NULL) {
    return NULL;
  }
  set = (lizard_surface_scope_set_t *)lizard_heap_alloc(sizeof(*set));
  set->head = NULL;
  set->count = 0UL;
  set->summary = 0UL;
  return set;
}

static lizard_surface_scope_set_t *scope_set_clone(
    lizard_heap_t *heap, const lizard_surface_scope_set_t *set) {
  lizard_surface_scope_set_t *out;
  out = scope_set_alloc(heap);
  if (out == NULL) {
    return NULL;
  }
  if (set != NULL) {
    out->head = set->head;
    out->count = set->count;
    out->summary = set->summary;
  }
  return out;
}

static lizard_surface_scope_set_t *scope_set_clone_without(
    lizard_heap_t *heap, const lizard_surface_scope_set_t *set,
    unsigned long removed) {
  lizard_surface_scope_set_t *out;
  lizard_surface_scope_node_t *iter;

  out = scope_set_alloc(heap);
  if (out == NULL) {
    return NULL;
  }
  if (set == NULL) {
    return out;
  }
  for (iter = set->head; iter != NULL; iter = iter->next) {
    if (iter->scope != removed) {
      out = lizard_surface_scope_set_add(heap, out, iter->scope);
      if (out == NULL) {
        return NULL;
      }
    }
  }
  return out;
}

static lizard_surface_property_t *surface_find_property(
    const lizard_surface_term_t *term, const char *key,
    lizard_surface_property_t **out_prev) {
  lizard_surface_property_t *cur;
  lizard_surface_property_t *prev;

  if (out_prev != NULL) {
    *out_prev = NULL;
  }
  if (term == NULL || key == NULL) {
    return NULL;
  }

  prev = NULL;
  cur = term->properties;
  while (cur != NULL) {
    if (cur->key != NULL && strcmp(cur->key, key) == 0) {
      if (out_prev != NULL) {
        *out_prev = prev;
      }
      return cur;
    }
    prev = cur;
    cur = cur->next;
  }
  return NULL;
}

lizard_surface_term_t *lizard_surface_from_ast(lizard_heap_t *heap,
                                               lizard_ast_node_t *ast) {
  lizard_source_span_t span;
  span_clear(&span);
  return lizard_surface_from_ast_span(heap, ast, span);
}

lizard_surface_term_t *lizard_surface_from_ast_span(lizard_heap_t *heap,
                                                    lizard_ast_node_t *ast,
                                                    lizard_source_span_t span) {
  lizard_surface_term_t *term;
  term = (lizard_surface_term_t *)lizard_heap_alloc(sizeof(*term));
  term->kind = ast != NULL ? LIZARD_SURFACE_DATUM : LIZARD_SURFACE_ERROR;
  term->heap = heap;
  term->ast = ast;
  term->span = span;
  term->phase = 0;
  term->scopes = lizard_surface_scope_set_empty(heap);
  term->properties = NULL;
  return term;
}


lizard_surface_term_t *lizard_surface_from_ast_like(
    lizard_heap_t *heap, lizard_ast_node_t *ast,
    const lizard_surface_term_t *origin) {
  lizard_source_span_t span;
  lizard_surface_term_t *term;

  span_clear(&span);
  if (origin != NULL) {
    span = origin->span;
  } else if (ast != NULL) {
    span = ast->span;
  }

  term = lizard_surface_from_ast_span(heap, ast, span);
  if (term == NULL) {
    return NULL;
  }
  if (origin != NULL) {
    (void)lizard_surface_copy_metadata(heap, term, origin, 1);
  }
  return term;
}

lizard_surface_term_t *lizard_surface_from_quoted_datum(
    lizard_heap_t *heap, lizard_ast_node_t *datum,
    const lizard_surface_term_t *origin) {
  lizard_ast_node_t *ast;

  if (heap == NULL) {
    return NULL;
  }
  ast = lizard_reparse_datum(datum, heap);
  if (ast == NULL) {
    ast = datum;
  }
  return lizard_surface_from_ast_like(heap, ast, origin);
}

int lizard_surface_copy_scopes(lizard_heap_t *heap,
                               lizard_surface_term_t *dst,
                               const lizard_surface_term_t *src) {
  lizard_surface_scope_set_t *copy;

  if (heap == NULL || dst == NULL || src == NULL) {
    return 0;
  }
  copy = scope_set_clone(heap, src->scopes);
  if (copy == NULL) {
    return 0;
  }
  dst->scopes = copy;
  return 1;
}

int lizard_surface_copy_properties(lizard_heap_t *heap,
                                   lizard_surface_term_t *dst,
                                   const lizard_surface_term_t *src) {
  lizard_surface_property_t *iter;

  if (heap == NULL || dst == NULL || src == NULL) {
    return 0;
  }
  for (iter = src->properties; iter != NULL; iter = iter->next) {
    if (iter->key != NULL) {
      if (!lizard_surface_property_set(heap, dst, iter->key, iter->value)) {
        return 0;
      }
    }
  }
  return 1;
}

int lizard_surface_copy_metadata(lizard_heap_t *heap,
                                 lizard_surface_term_t *dst,
                                 const lizard_surface_term_t *src,
                                 int copy_properties) {
  if (heap == NULL || dst == NULL || src == NULL) {
    return 0;
  }
  dst->span = src->span;
  dst->phase = src->phase;
  if (!lizard_surface_copy_scopes(heap, dst, src)) {
    return 0;
  }
  if (copy_properties) {
    return lizard_surface_copy_properties(heap, dst, src);
  }
  return 1;
}

static const char *surface_kind_name(lizard_surface_kind_t kind) {
  switch (kind) {
  case LIZARD_SURFACE_DATUM:
    return "datum";
  case LIZARD_SURFACE_ERROR:
    return "error";
  }
  return "unknown";
}

char *lizard_surface_debug_string(lizard_heap_t *heap,
                                  const lizard_surface_term_t *term) {
  char *out;
  const char *filename;
  int line;
  int column;
  int phase;
  unsigned long scope_count;
  unsigned long scope_summary;
  const char *kind;

  if (heap == NULL) {
    return NULL;
  }

  if (term == NULL) {
    out = (char *)lizard_heap_alloc(48U);
    sprintf(out, "#<surface null>");
    return out;
  }

  filename = term->span.filename != NULL ? term->span.filename : "<unknown>";
  line = term->span.start_line;
  column = term->span.start_column;
  phase = term->phase;
  scope_count = lizard_surface_scope_set_count(term->scopes);
  scope_summary = lizard_surface_scope_set_summary(term->scopes);
  kind = surface_kind_name(term->kind);

  out = (char *)lizard_heap_alloc(384U);
  sprintf(out,
          "#<surface kind=%s phase=%d scopes=%lu summary=%lu at %s:%d:%d>",
          kind, phase, scope_count, scope_summary, filename, line, column);
  return out;
}

lizard_surface_kind_t lizard_surface_kind(const lizard_surface_term_t *term) {
  return term != NULL ? term->kind : LIZARD_SURFACE_ERROR;
}

lizard_ast_node_t *lizard_surface_ast(const lizard_surface_term_t *term) {
  return term != NULL ? term->ast : NULL;
}

int lizard_surface_span(const lizard_surface_term_t *term,
                        lizard_source_span_t *out_span) {
  if (out_span == NULL) {
    return 0;
  }
  if (term == NULL) {
    span_clear(out_span);
    return 0;
  }
  *out_span = term->span;
  return 1;
}


lz_list_t *lizard_surface_list_from_ast_list(lizard_heap_t *heap,
                                             lz_list_t *ast_list) {
  lz_list_t *surface_list;
  lz_list_node_t *iter;

  if (heap == NULL || ast_list == NULL) {
    return NULL;
  }

  surface_list = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (iter = ast_list->head; iter != ast_list->nil; iter = iter->next) {
    lizard_ast_list_node_t *ast_node;
    lizard_surface_list_node_t *surface_node;
    lizard_ast_node_t *ast;
    lizard_source_span_t span;

    ast_node = (lizard_ast_list_node_t *)iter;
    ast = ast_node->ast;
    span_clear(&span);
    if (ast != NULL) {
      span = ast->span;
    }
    surface_node =
        (lizard_surface_list_node_t *)lizard_heap_alloc(sizeof(*surface_node));
    surface_node->term = lizard_surface_from_ast_span(heap, ast, span);
    list_append(surface_list, &surface_node->node);
  }
  return surface_list;
}

lz_list_t *lizard_surface_parse_source(lizard_heap_t *heap,
                                       const char *source,
                                       const char *filename,
                                       lizard_diagnostic_t *diagnostic) {
  lz_list_t *tokens;
  lz_list_t *ast_list;
  const lizard_diagnostic_t *last;

  if (heap == NULL || source == NULL) {
    diagnostic_parse_error(diagnostic, "null source");
    return NULL;
  }

  diagnostic_clear(diagnostic);
  tokens = lizard_tokenize_source(source, filename, diagnostic);
  if (tokens == NULL) {
    last = lizard_tokenizer_last_diagnostic();
    diagnostic_copy(diagnostic, last);
    if (diagnostic != NULL && diagnostic->status == LIZARD_STATUS_OK) {
      diagnostic->status = LIZARD_STATUS_PARSE_ERROR;
    }
    return NULL;
  }

  ast_list = lizard_parse(tokens, heap);
  if (ast_list == NULL) {
    last = lizard_parser_last_diagnostic();
    diagnostic_copy(diagnostic, last);
    if (diagnostic != NULL && diagnostic->status == LIZARD_STATUS_OK) {
      diagnostic->status = LIZARD_STATUS_PARSE_ERROR;
    }
    return NULL;
  }

  diagnostic_clear(diagnostic);
  return lizard_surface_list_from_ast_list(heap, ast_list);
}

int lizard_surface_phase(const lizard_surface_term_t *term) {
  return term != NULL ? term->phase : 0;
}

void lizard_surface_set_phase(lizard_surface_term_t *term, int phase) {
  if (term != NULL) {
    term->phase = phase;
  }
}

lizard_surface_scope_set_t *lizard_surface_scope_set_empty(lizard_heap_t *heap) {
  return scope_set_alloc(heap);
}

lizard_surface_scope_set_t *lizard_surface_scope_set_add(
    lizard_heap_t *heap, const lizard_surface_scope_set_t *set,
    unsigned long scope) {
  lizard_surface_scope_set_t *out;
  lizard_surface_scope_node_t *node;

  if (heap == NULL) {
    return NULL;
  }
  if (lizard_surface_scope_set_contains(set, scope)) {
    return scope_set_clone(heap, set);
  }

  out = scope_set_alloc(heap);
  if (out == NULL) {
    return NULL;
  }
  if (set != NULL) {
    out->head = set->head;
    out->count = set->count;
    out->summary = set->summary;
  }

  node = (lizard_surface_scope_node_t *)lizard_heap_alloc(sizeof(*node));
  node->scope = scope;
  node->next = out->head;
  out->head = node;
  out->count++;
  out->summary ^= scope_hash(scope);
  return out;
}

lizard_surface_scope_set_t *lizard_surface_scope_set_remove(
    lizard_heap_t *heap, const lizard_surface_scope_set_t *set,
    unsigned long scope) {
  if (heap == NULL) {
    return NULL;
  }
  if (!lizard_surface_scope_set_contains(set, scope)) {
    return scope_set_clone(heap, set);
  }
  return scope_set_clone_without(heap, set, scope);
}

int lizard_surface_scope_set_contains(const lizard_surface_scope_set_t *set,
                                      unsigned long scope) {
  lizard_surface_scope_node_t *iter;
  if (set == NULL) {
    return 0;
  }
  for (iter = set->head; iter != NULL; iter = iter->next) {
    if (iter->scope == scope) {
      return 1;
    }
  }
  return 0;
}

int lizard_surface_scope_set_equal(const lizard_surface_scope_set_t *a,
                                   const lizard_surface_scope_set_t *b) {
  lizard_surface_scope_node_t *iter;
  if (a == b) {
    return 1;
  }
  if (a == NULL || b == NULL) {
    return lizard_surface_scope_set_count(a) ==
           lizard_surface_scope_set_count(b);
  }
  if (a->count != b->count || a->summary != b->summary) {
    return 0;
  }
  for (iter = a->head; iter != NULL; iter = iter->next) {
    if (!lizard_surface_scope_set_contains(b, iter->scope)) {
      return 0;
    }
  }
  return 1;
}

unsigned long lizard_surface_scope_set_count(
    const lizard_surface_scope_set_t *set) {
  return set != NULL ? set->count : 0UL;
}

unsigned long lizard_surface_scope_set_summary(
    const lizard_surface_scope_set_t *set) {
  return set != NULL ? set->summary : 0UL;
}

const lizard_surface_scope_set_t *lizard_surface_scopes(
    const lizard_surface_term_t *term) {
  return term != NULL ? term->scopes : NULL;
}

void lizard_surface_set_scopes(lizard_surface_term_t *term,
                               lizard_surface_scope_set_t *scopes) {
  if (term != NULL) {
    term->scopes = scopes;
  }
}

unsigned long lizard_surface_scope(const lizard_surface_term_t *term) {
  return term != NULL ? lizard_surface_scope_set_summary(term->scopes) : 0UL;
}

void lizard_surface_add_scope(lizard_surface_term_t *term, unsigned long scope) {
  lizard_surface_scope_set_t *updated;
  if (term != NULL) {
    updated = lizard_surface_scope_set_add(term->heap, term->scopes, scope);
    if (updated != NULL) {
      term->scopes = updated;
    }
  }
}

void lizard_surface_remove_scope(lizard_surface_term_t *term,
                                 unsigned long scope) {
  lizard_surface_scope_set_t *updated;
  if (term != NULL) {
    updated = lizard_surface_scope_set_remove(term->heap, term->scopes, scope);
    if (updated != NULL) {
      term->scopes = updated;
    }
  }
}

int lizard_surface_has_scope(const lizard_surface_term_t *term,
                             unsigned long scope) {
  return term != NULL ? lizard_surface_scope_set_contains(term->scopes, scope)
                      : 0;
}

int lizard_surface_same_scopes(const lizard_surface_term_t *a,
                               const lizard_surface_term_t *b) {
  return lizard_surface_scope_set_equal(lizard_surface_scopes(a),
                                        lizard_surface_scopes(b));
}

void lizard_surface_clear_scopes(lizard_surface_term_t *term) {
  if (term != NULL) {
    term->scopes = lizard_surface_scope_set_empty(term->heap);
  }
}

int lizard_surface_property_set(lizard_heap_t *heap,
                                lizard_surface_term_t *term,
                                const char *key,
                                lizard_ast_node_t *value) {
  lizard_surface_property_t *prop;
  const char *owned_key;

  if (heap == NULL || term == NULL || key == NULL) {
    return 0;
  }

  prop = surface_find_property(term, key, NULL);
  if (prop != NULL) {
    prop->value = value;
    return 1;
  }

  owned_key = surface_copy_key(heap, key);
  if (owned_key == NULL) {
    return 0;
  }

  prop = (lizard_surface_property_t *)lizard_heap_alloc(sizeof(*prop));
  prop->next = term->properties;
  prop->key = owned_key;
  prop->value = value;
  term->properties = prop;
  return 1;
}

lizard_ast_node_t *lizard_surface_property_get(const lizard_surface_term_t *term,
                                               const char *key) {
  lizard_surface_property_t *prop;
  prop = surface_find_property(term, key, NULL);
  return prop != NULL ? prop->value : NULL;
}

int lizard_surface_property_has(const lizard_surface_term_t *term,
                                const char *key) {
  return surface_find_property(term, key, NULL) != NULL;
}

int lizard_surface_property_remove(lizard_surface_term_t *term,
                                   const char *key) {
  lizard_surface_property_t *prop;
  lizard_surface_property_t *prev;

  if (term == NULL || key == NULL) {
    return 0;
  }

  prop = surface_find_property(term, key, &prev);
  if (prop == NULL) {
    return 0;
  }
  if (prev == NULL) {
    term->properties = prop->next;
  } else {
    prev->next = prop->next;
  }
  return 1;
}
