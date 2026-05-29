# Stable C API foundation

This snapshot adds `include/lizard_api.h`, an embeddable API that hides the
internal AST, heap, parser, evaluator, and `ds` implementation details from
normal embedders.

The API is intentionally small:

```c
#include <lizard_api.h>

lizard_runtime_t *rt = lizard_runtime_create(NULL);
lizard_context_t *ctx = lizard_context_create(rt);
lizard_value_t *v = NULL;

if (lizard_context_eval_string(ctx, "(+ 40 2)", &v) == LIZARD_STATUS_OK) {
  lizard_value_print(v);
}

lizard_context_destroy(ctx);
lizard_runtime_destroy(rt);
```

## Current guarantee

The public API gives embedders a stable shape:

- `lizard_runtime_t` owns heap-scale runtime state.
- `lizard_context_t` owns an environment and last-result metadata.
- `lizard_value_t` is opaque to API users.
- API users can evaluate strings/files and print/query values.
- API users can inspect the last structured diagnostic.
- API users can ask values for their source span when available.

## Known limitation

The implementation still has a legacy process-global active heap because the
old GMP allocation hooks and arena allocator depend on it. The new runtime API
centralizes that global and makes the future migration path explicit: move the
active heap, diagnostics, continuation state, module registry, and GC roots into
`lizard_runtime_t` without changing API users.

## Next steps

- Make the runtime reentrant by removing all remaining process-global evaluator
  state.
- Add a value-retention/handle API before adding moving or compacting GC.
- Continue shrinking the compatibility `include/lizard.h` wrapper until embedders
  only need `include/lizard_api.h`.
- Replace remaining parser/evaluator fatal exits with structured diagnostics.

## Diagnostic metadata

`lizard_diagnostic_t` now includes diagnostic severity and source category:

```c
lizard_diagnostic_severity_t severity;
lizard_diagnostic_category_t category;
```

Use `lizard_diagnostic_severity_name` and `lizard_diagnostic_category_name` for
tooling output instead of inferring categories from diagnostic text.

## Diagnostic and syntax expansion reports

The stable C API now exposes owned diagnostic report snapshots:

```c
lizard_diagnostic_report_t *lizard_context_diagnostic_report(
    lizard_context_t *context);
void lizard_diagnostic_report_destroy(lizard_diagnostic_report_t *report);
```

Diagnostic report v2 includes status, severity, category, source location, and
message data.  Text and JSON writers are available through
`lizard_diagnostic_report_fprint` and `lizard_diagnostic_report_fprint_json`.

The syntax expansion report API gives tools a parse/expand-only surface and can
also produce an owned diagnostic report on failure.

## Report and syntax-tooling boundary

The stable public API exports all report/event handle types needed by syntax and
reporting headers:

```c
lizard_expansion_trace_report_t;
lizard_expansion_trace_event_t;
lizard_diagnostic_report_t;
lizard_diagnostic_event_t;
lizard_syntax_expansion_report_t;
```

Internal headers must not expose prototypes involving types that are absent from
`include/lizard_api.h`.  Use `make api-audit` to check the public report/type
boundary.

## Diagnostic construction helpers

The public API now exposes helper functions for constructing diagnostics and
source spans consistently:

```c
void lizard_source_span_clear(lizard_source_span_t *span);
void lizard_source_span_set(lizard_source_span_t *span, const char *filename,
                            int start_line, int start_column,
                            int end_line, int end_column,
                            int start_offset, int end_offset);
void lizard_diagnostic_clear(lizard_diagnostic_t *diagnostic);
void lizard_diagnostic_set(lizard_diagnostic_t *diagnostic,
                           lizard_status_t status,
                           lizard_diagnostic_category_t category,
                           const lizard_source_span_t *span,
                           const char *message);
void lizard_diagnostic_set_simple(lizard_diagnostic_t *diagnostic,
                                  lizard_status_t status,
                                  lizard_diagnostic_category_t category,
                                  const char *message);
void lizard_diagnostic_copy(lizard_diagnostic_t *dst,
                            const lizard_diagnostic_t *src);
```

Embedders can use these helpers to build diagnostics with the same severity,
category, span-clearing, and bounded-message behavior used internally.
