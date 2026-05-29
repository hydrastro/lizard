# Representation Boundary Scaffold

Phase 2A introduces the explicit representation boundary used by the proof and
language-oriented roadmap.  This is scaffolding only: the evaluator still runs
existing `lizard_ast_node_t` values, and the trusted kernel still checks
`kterm_t` values.

The intended split is:

```text
SurfaceTerm   parsed/expanded syntax with source locations, phase, and scope
CoreTerm      elaborator output; explicit, but still untrusted
KernelTerm    trusted kernel term (`kterm_t` today)
Value         runtime value evaluated by the Lisp runtime/VM
```

## Trust boundary

`SurfaceTerm` and `CoreTerm` are untrusted.  Reader, macro expansion, future
`#lang` front-ends, elaboration, tactics, CAS, and libraries may produce them.
The kernel must not trust them.  A future erasure/indexing phase will convert a
checked fragment of `CoreTerm` to `KernelTerm`, and the kernel will remain the
final authority.

## Current implementation

- `src/surface_term.h/.c` wraps an existing AST with source span, phase, and a
  simple scope-token accumulator.
- `src/core_term.h/.c` wraps either a runtime AST, a kernel term, or a hole.
- `tests/term_boundary_test.c` proves these are distinct wrappers today, before
  changing evaluator semantics.

## Next steps

1. Make the reader produce `SurfaceTerm` directly.
2. Preserve `SurfaceTerm` scope/source data through macro expansion.
3. Add a minimal elaborator that produces `CoreTerm` instead of raw AST.
4. Add a converter from checked `CoreTerm` to `KernelTerm`.
5. Keep runtime `Value` separate from all three syntax/proof representations.

## C89 API note

The boundary APIs avoid returning aggregate structs by value.  Accessors such as
`lizard_surface_span` write into caller-provided out-parameters so the project
continues to build under `-Waggregate-return -Werror`.

## Phase 2B: parser-to-surface boundary

Phase 2B adds the first real use of `SurfaceTerm` without changing evaluator
semantics. Existing parser output is still `lizard_ast_node_t`, but callers can
now wrap parsed top-level forms as `SurfaceTerm` values while preserving source
locations:

```c
lizard_diagnostic_t diagnostic;
lz_list_t *forms = lizard_surface_parse_source(heap, source, filename,
                                               &diagnostic);
```

Each returned `lizard_surface_list_node_t` holds one untrusted surface term. The
term points at the existing runtime AST and carries the AST span copied from the
tokenizer/parser path. This gives future reader, macro, module, and elaborator
work a typed boundary to consume while keeping the old evaluator path stable.

Important: this is still not a trusted kernel path. A `SurfaceTerm` may become
a `CoreTerm`, but the kernel must continue to accept only checked kernel terms.


## Phase 2C syntax-object metadata

SurfaceTerm is now also the home of the initial syntax-object metadata layer.
This metadata is untrusted and is ignored by the evaluator and kernel.  It is
there so later reader, macro, module, contract, and tooling phases can preserve
source-aware information instead of attaching ad-hoc side channels.

Current metadata:

- `phase`: integer expansion/runtime phase marker;
- `scope`: compact unsigned-long scope marker;
- properties: string-keyed table carrying AST values for tool/macro metadata.

The compact `scope` marker is deliberately temporary.  Full hygiene needs a
persistent scope-set representation, but this phase gives us the API boundary
and tests before changing macro expansion.


### Phase 2D update

`SurfaceTerm` now carries an explicit scope-set object rather than only a
single marker.  The old `lizard_surface_scope()` function remains as a compact
summary for compatibility, but macro-expansion work should use the explicit
scope-set APIs for containment and equality.

## Phase 2E: transformation-preserving surface helpers

The parser-to-surface boundary can now preserve metadata through wrapper
transformations.  This is still untrusted syntax metadata, not evaluator state
and not kernel state.  The intended future macro-expansion pattern is:

```text
input SurfaceTerm
  -> transform AST/datum
  -> lizard_surface_from_ast_like / lizard_surface_from_quoted_datum
  -> output SurfaceTerm with span/phase/scopes/properties propagated
```

This prepares the syntax layer for macro expansion, syntax debugging, and
source-preserving rewrites without changing the runtime AST evaluator path.
