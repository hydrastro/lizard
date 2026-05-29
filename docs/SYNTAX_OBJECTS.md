# Syntax object scaffold

Phase 2C does not implement full Racket-style syntax objects yet.  It adds the
minimal metadata carrier needed to grow that system without disturbing the
runtime evaluator.

A current `lizard_surface_term_t` is still a wrapper around the existing runtime
AST, but it now carries:

- source span;
- phase level;
- a compact lexical-scope marker;
- a property table for macro/debugger/tool metadata.

This makes the boundary explicit:

```text
reader/parser       -> SurfaceTerm
macro expander      -> SurfaceTerm -> SurfaceTerm
elaborator          -> SurfaceTerm -> CoreTerm
kernel conversion   -> CoreTerm -> KernelTerm
trusted kernel      -> checks KernelTerm
runtime evaluator   -> evaluates Value
```

The property table is intentionally small and untrusted.  It is not part of the
kernel, and the evaluator does not consult it.  Future macro expansion can use
it for origin tracking, expansion-step recording, source-preserving rewrites,
contract blame annotations, and tool/debugger metadata.

The current scope representation is a single marker set encoded as an unsigned
long.  That is enough for Phase 2 scaffolding and tests, but it is not the final
hygiene representation.  A later phase should replace it with a persistent
scope-set object.


## Phase 2D: explicit scope-set scaffold

The first syntax-object scaffold used a single unsigned-long scope marker as a
compact placeholder.  Phase 2D replaces that with an explicit small persistent
scope-set object while preserving `lizard_surface_scope()` as a compatibility
summary/hash for existing callers.

The scope-set API is deliberately simple:

```c
lizard_surface_scope_set_t *lizard_surface_scope_set_add(heap, set, scope);
lizard_surface_scope_set_t *lizard_surface_scope_set_remove(heap, set, scope);
int lizard_surface_scope_set_contains(set, scope);
int lizard_surface_scope_set_equal(a, b);
```

Surface terms expose convenience mutators:

```c
lizard_surface_add_scope(term, scope);
lizard_surface_remove_scope(term, scope);
lizard_surface_has_scope(term, scope);
lizard_surface_same_scopes(a, b);
```

These operations are still untrusted metadata.  They are not consulted by the
kernel or evaluator.  They exist so the future macro expander can move from a
single hygiene placeholder toward real scope-set comparison and propagation.

## Phase 2E: metadata propagation hooks

Phase 2E adds the first transformation-oriented syntax-object helpers.  They do
not change macro expansion or evaluation; they give future macro and reader
passes a disciplined way to preserve untrusted metadata across rewrites.

New helpers include:

```c
lizard_surface_from_ast_like(heap, ast, origin);
lizard_surface_from_quoted_datum(heap, datum, origin);
lizard_surface_copy_scopes(heap, dst, src);
lizard_surface_copy_properties(heap, dst, src);
lizard_surface_copy_metadata(heap, dst, src, copy_properties);
lizard_surface_debug_string(heap, term);
```

`lizard_surface_from_ast_like` constructs a new surface wrapper around a new AST
while copying source span, phase, scopes, and properties from an origin term.
`lizard_surface_from_quoted_datum` additionally reparses quoted Lisp datum into
specialized AST form before wrapping it.  This is useful for macro-output and
reader-output paths that start from datum values rather than already-specialized
AST nodes.

`lizard_surface_debug_string` is intentionally non-normative.  It is a compact
human/debugging aid for tests, macro steppers, and future tooling; the kernel and
evaluator must not depend on it.

## Phase 2F: transformation tracing

Surface terms now carry an untrusted transformation trace chain.  The trace is
intended for a future macro stepper, rewrite debugger, and source-aware tooling.
It is deliberately ignored by the evaluator and by the trusted kernel.

Current trace metadata records:

- the stage name, such as `reader`, `macro-expand`, or `rewrite`;
- a short detail string;
- the origin source filename/line/column when available;
- the origin phase and scope summary.

Important APIs:

```c
int lizard_surface_trace_add(lizard_heap_t *heap,
                             lizard_surface_term_t *term,
                             const char *stage,
                             const char *detail,
                             const lizard_surface_term_t *origin);

int lizard_surface_trace_copy(lizard_heap_t *heap,
                              lizard_surface_term_t *dst,
                              const lizard_surface_term_t *src);

char *lizard_surface_trace_debug_string(lizard_heap_t *heap,
                                        const lizard_surface_term_t *term);
```

The trace chain is copied by `lizard_surface_copy_metadata` and by the
`lizard_surface_from_ast_like` helper so future expansion/rewrite passes can
produce explainable output syntax objects.

## Phase 2G: expansion context scaffold

Phase 2G adds an untrusted expansion context object for future hygienic macro
expansion and macro debugging.  The context does not run the macro expander and
is not consulted by the evaluator or kernel.  It provides the state that a
future expander will need:

- current phase;
- context/debug name;
- fresh macro scope allocation;
- helpers for marking syntax as introduced by a macro;
- helpers for recording rewrite/expansion trace events.

Important APIs:

```c
lizard_expansion_context_t *lizard_expansion_context_new(heap, phase, name);
unsigned long lizard_expansion_context_fresh_scope(context);
int lizard_expansion_context_introduce(context, term, origin,
                                       macro_name, detail, &scope);
int lizard_expansion_context_rewrite(context, term, origin, stage, detail);
char *lizard_surface_origin_chain_debug_string(heap, term);
```

`lizard_expansion_context_introduce` adds a fresh scope to the output surface
term, sets its phase to the context phase, and appends a `macro-introduce`
trace event.  This is the first explicit bridge from syntax-object metadata to
a future macro stepper.

## Phase 2H: macro-expander adapter scaffold

`syntax_expander.c` introduces the first bridge from syntax-object metadata into
the existing macro expander. The adapter accepts a `SurfaceTerm` and an
`ExpansionContext`, delegates expansion to the old AST macro expander, and wraps
the expanded AST back into a `SurfaceTerm` using the input syntax object as the
metadata origin.

This is deliberately not a new macro expander. It records untrusted
`macro-expand` trace events and preserves source spans, phase, scope sets, and
properties, while keeping runtime AST behavior unchanged.


## Runtime trace hook

With Phase 2I, a context can opt into expansion tracing.  The traced path records
the same `SurfaceTerm` metadata used by the macro-adapter scaffold while still
evaluating the old expanded runtime AST.  This is intended for tooling and
diagnostics; the kernel and evaluator continue to ignore syntax metadata.


## Phase 2J: reportable trace events

SurfaceTerm transformation traces can now be queried event-by-event and formatted
into bounded caller-provided buffers.  This is the first API shape suitable for
future editor/macro-stepper integration because callers can ask for every
recorded stage rather than only the latest event.


## CLI trace hooks

`build/lizard --trace-expansion --print-expansion-trace --eval EXPR` runs the
existing evaluator through the optional traced expansion path and prints an owned
trace report. Normal execution does not print trace data.

## Phase 2M: file/export trace reports

Phase 2M adds `--trace-expansion-file PATH` and
`lizard_expansion_trace_report_fprint`.  The export format is line-oriented for
external tools:

```text
lizard-expansion-trace\tv=1\tcount=N
event\tindex=0\tstage=macro-expand\tdetail=...\torigin=<string>\tline=1\tcolumn=1\tphase=0\tscope=...
```

Text fields escape tabs, newlines, carriage returns, and backslashes.  Exporting
a report remains opt-in; macro expansion and evaluator semantics are unchanged.

## Phase 2N: expand-only reports

The CLI now has `--expand-only`, backed by `lizard_syntax_expansion_report_t`.
It runs the SurfaceTerm parser and syntax-expander adapter, records the
transformation trace, prints the expanded AST summary, and stops before
evaluation. Macro and evaluator semantics are unchanged.



## Phase 2O: JSON expansion reports

`--expand-only-format text|json` keeps the existing text format as v1 and adds a JSON-safe report writer for editor integration. JSON output is still opt-in and does not evaluate the program.

## Phase 2P: diagnostic reports

Expansion reports now include an owned diagnostic report.  On parse/tokenizer or
expansion failure, the text and JSON report writers include the diagnostic
status, message, filename, line/column, and offsets in a tooling-friendly
format.  This keeps `--expand-only` useful for editors even when expansion
fails.

## Report schema stability

Syntax expansion reports, expansion trace reports, and diagnostic reports use
the shared report schema registry for their type names and version numbers.
This keeps editor/tooling integrations independent from local writer
implementation details.

