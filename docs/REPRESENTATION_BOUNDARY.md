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

## Phase 2F tracing boundary

SurfaceTerm now also has untrusted transformation-trace metadata.  This is the
first scaffold for a future syntax debugger / macro stepper.  Trace metadata is
not part of CoreTerm, KernelTerm, or runtime Value semantics; it is an
explanation layer attached to surface syntax only.

This preserves the desired trust boundary:

```text
SurfaceTerm: source spans, phase, scopes, properties, traces
CoreTerm:    future elaborator output, still untrusted
KernelTerm:  trusted de-Bruijn checked term
Value:       runtime evaluator object
```

## Phase 2G: expansion-context scaffold

The SurfaceTerm layer now has an explicit expansion-context scaffold.  This
provides fresh macro scopes and trace hooks for future macro expansion without
changing evaluator behavior.

The trust boundary remains unchanged:

```text
ExpansionContext + SurfaceTerm metadata: untrusted tooling/debug data
CoreTerm: future elaborator output, still untrusted
KernelTerm: trusted checked representation
Value: runtime evaluator representation
```

A future macro expander can now allocate a scope, attach it to introduced
syntax, and record a trace event using one context-owned API instead of directly
mutating the surface object in ad-hoc ways.

## Phase 2H adapter boundary

The syntax-expander adapter is the first executable bridge from the old AST
macro expansion path into the new representation-boundary scaffold:

```text
SurfaceTerm + ExpansionContext
  -> existing lizard_expand_macros(AST)
  -> expanded runtime AST
  -> SurfaceTerm wrapper with copied metadata and a macro-expand trace
```

The evaluator still consumes runtime AST values. The adapter exists so future
hygienic expansion can be introduced behind a stable API while existing macro
semantics remain testable.


## Phase 2I: Optional runtime expansion tracing

The runtime API now has an opt-in traced expansion mode.  It does not change
default evaluation.  When enabled on a `lizard_context_t`, evaluation wraps
parsed forms as `SurfaceTerm`, expands through the adapter, stores the latest
expanded surface term, and exposes trace metadata through public API accessors.

This is a bridge for tools: REPLs, editors, macro steppers, and diagnostics can
observe expansion provenance without trusting it and without making the evaluator
consume syntax objects directly.


## Phase 2J: expansion trace reports

The traced expansion path now exposes structured trace events through the public
context API.  The old `last_expansion_stage/detail/span` helpers remain as
convenience accessors; embedders that need macro-stepper-like information should
use the indexed trace event API.  Index `0` is the newest event, matching the
existing "latest" helpers.

The trace report remains untrusted metadata.  The evaluator and kernel continue
to ignore it.


## Phase 2K: owned trace reports

Runtime expansion tracing originally exposed borrowed views into the latest
expanded `SurfaceTerm`. Phase 2K adds `lizard_expansion_trace_report_t`, an
owned snapshot object. This is the safer API for editors, REPL tooling, and
macro steppers: callers can capture a report, continue evaluating, or destroy
the context without invalidating the report.

Reports are still untrusted debugging metadata. The evaluator and kernel do not
consult them.


## Phase 2L: REPL trace reporting

The command-line executable can now opt into traced expansion and print the
owned expansion trace report. This exposes SurfaceTerm metadata to tooling while
leaving the evaluator path unchanged by default.

## Phase 2M: trace export boundary

Trace reports now have a tooling-facing export boundary.
`lizard_expansion_trace_report_fprint` writes an owned report in a stable
line-oriented format, and the REPL exposes it through
`--trace-expansion-file PATH`.  This keeps expansion metadata observable without
changing the runtime value/evaluator boundary.

## Phase 2N: syntax expansion reports

`lizard_syntax_expansion_report_t` snapshots parse and macro-expansion output
without evaluating it. Reports own their strings and expose status, diagnostics,
source span, phase, scope summary, expanded AST summary, and trace events. This
keeps editor/macro-stepper tooling independent from the evaluator.



## Phase 2O: report formats

Syntax expansion reports now have two stable writers: the existing line-oriented text format and a JSON writer. Both are generated from owned report snapshots, not borrowed context internals.

## Phase 2P: Diagnostic report snapshots

Diagnostics now have an owned report layer parallel to expansion traces.
`lizard_context_diagnostic_report` and
`lizard_syntax_expansion_report_diagnostic_report` copy the current diagnostic
into a stable `lizard_diagnostic_report_t`.  Text and JSON writers make parser,
tokenizer, and expansion failures available to editors without borrowing mutable
context state.  Syntax expansion reports include this diagnostic report in their
text/JSON output, including `--expand-only` failures.

## Phase 2Q: shared report formatting

Expansion trace reports, syntax expansion reports, and diagnostic reports now
share `report_writer.c` for text-field and JSON-string escaping. This keeps
tooling-facing report output stable while the representation boundary evolves.

