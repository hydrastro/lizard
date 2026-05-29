/* src/expansion_context.c -- untrusted macro-expansion context scaffold.
 *
 * Phase 2G adds a small context object for future hygienic macro expansion.
 * It allocates fresh scopes, records phase/name metadata, and adds tracing
 * events to SurfaceTerm values. It deliberately does not change evaluation or
 * macro-expansion semantics yet.
 */

#include "expansion_context.h"
#include "mem.h"

#include <stdio.h>
#include <string.h>

struct lizard_expansion_context {
  lizard_heap_t *heap;
  int phase;
  unsigned long next_scope;
  unsigned long scope_base;
  const char *name;
};

static const char *copy_string(lizard_heap_t *heap, const char *text) {
  char *copy;
  size_t len;

  if (heap == NULL || text == NULL) {
    return NULL;
  }
  len = strlen(text) + 1U;
  copy = (char *)lizard_heap_alloc(len);
  memcpy(copy, text, len);
  return copy;
}

static char *join_detail(lizard_heap_t *heap, const char *macro_name,
                         const char *detail) {
  char *out;
  size_t macro_len;
  size_t detail_len;
  size_t total;

  if (heap == NULL) {
    return NULL;
  }
  if (macro_name == NULL) {
    macro_name = "<macro>";
  }
  if (detail == NULL) {
    detail = "";
  }
  macro_len = strlen(macro_name);
  detail_len = strlen(detail);
  total = macro_len + detail_len + 4U;
  out = (char *)lizard_heap_alloc(total);
  if (detail_len == 0U) {
    sprintf(out, "%s", macro_name);
  } else {
    sprintf(out, "%s: %s", macro_name, detail);
  }
  return out;
}

lizard_expansion_context_t *lizard_expansion_context_new(
    lizard_heap_t *heap, int phase, const char *name) {
  lizard_expansion_context_t *context;

  if (heap == NULL) {
    return NULL;
  }
  context = (lizard_expansion_context_t *)lizard_heap_alloc(sizeof(*context));
  context->heap = heap;
  context->phase = phase;
  context->next_scope = 1UL;
  context->scope_base = 0xE1000000UL;
  context->name = copy_string(heap, name != NULL ? name : "expansion");
  return context;
}

lizard_heap_t *lizard_expansion_context_heap(
    const lizard_expansion_context_t *context) {
  return context != NULL ? context->heap : NULL;
}

int lizard_expansion_context_phase(
    const lizard_expansion_context_t *context) {
  return context != NULL ? context->phase : 0;
}

void lizard_expansion_context_set_phase(lizard_expansion_context_t *context,
                                        int phase) {
  if (context != NULL) {
    context->phase = phase;
  }
}

const char *lizard_expansion_context_name(
    const lizard_expansion_context_t *context) {
  return context != NULL ? context->name : NULL;
}

unsigned long lizard_expansion_context_fresh_scope(
    lizard_expansion_context_t *context) {
  unsigned long scope;

  if (context == NULL) {
    return 0UL;
  }
  scope = context->scope_base + context->next_scope;
  context->next_scope++;
  if (context->next_scope == 0UL) {
    context->next_scope = 1UL;
  }
  return scope;
}

int lizard_expansion_context_introduce(
    lizard_expansion_context_t *context, lizard_surface_term_t *term,
    const lizard_surface_term_t *origin, const char *macro_name,
    const char *detail, unsigned long *out_scope) {
  unsigned long scope;
  char *joined;

  if (context == NULL || term == NULL) {
    return 0;
  }
  scope = lizard_expansion_context_fresh_scope(context);
  if (scope == 0UL) {
    return 0;
  }
  lizard_surface_add_scope(term, scope);
  lizard_surface_set_phase(term, context->phase);
  joined = join_detail(context->heap, macro_name, detail);
  if (joined == NULL) {
    return 0;
  }
  if (!lizard_surface_trace_add(context->heap, term, "macro-introduce",
                                joined, origin)) {
    return 0;
  }
  if (out_scope != NULL) {
    *out_scope = scope;
  }
  return 1;
}

int lizard_expansion_context_rewrite(
    lizard_expansion_context_t *context, lizard_surface_term_t *term,
    const lizard_surface_term_t *origin, const char *stage,
    const char *detail) {
  if (context == NULL || term == NULL || stage == NULL) {
    return 0;
  }
  lizard_surface_set_phase(term, context->phase);
  return lizard_surface_trace_add(context->heap, term, stage, detail, origin);
}

char *lizard_expansion_context_debug_string(
    lizard_expansion_context_t *context) {
  char *out;
  const char *name;
  int phase;
  unsigned long next_scope;

  if (context == NULL || context->heap == NULL) {
    return NULL;
  }
  name = context->name != NULL ? context->name : "<unnamed>";
  phase = context->phase;
  next_scope = context->scope_base + context->next_scope;
  out = (char *)lizard_heap_alloc(256U);
  sprintf(out, "#<expansion-context name=%s phase=%d next-scope=%lu>",
          name, phase, next_scope);
  return out;
}
