
## Report API visibility

Tests and embedders must include only `lizard_api.h`.  Any public report object
used by tests must have its opaque typedef and prototypes declared there; do not
make tests include internal headers to compensate for missing API declarations.

## Public API boundary audit

Headers under `src/` may mention public report/event types only if those types
are guaranteed by `include/lizard_api.h`.  Run `make api-audit` after touching
report, syntax-object, diagnostic, or expansion-trace APIs.  The audit checks
that opaque report handles and public event structs remain visible to embedders
and internal headers alike.

## Header boundary audit

Any implementation header under `src/` that exposes public API types must include
`lizard_api.h` directly. Do not rely on include order from another header or C
file. Run `make header-audit` after touching syntax-object, report, diagnostic,
or public API boundary headers.

## Include layering

Headers must not rely on accidental transitive includes.  If a header exposes a
public API type, it includes `lizard_api.h` directly.  Public headers must not
include implementation headers, and implementation header dependencies must stay
acyclic.  Use `make header-audit` and `make include-audit` before merging.


## Diagnostic construction

All new diagnostics must be built through the canonical helpers in
`diagnostics.c` / `diagnostics.h`.  Do not hand-initialize diagnostic fields in
new tokenizer/parser/runtime/report code.  Use `lizard_diagnostic_set` when a
source span is known and `lizard_diagnostic_set_simple` when there is no span.
This keeps status, severity, category, span, and bounded message-copy behavior
consistent under the strict warning profile.

## Ownership audit

`make ownership-audit` is part of the runtime-hardening gate.  Direct AST heap
allocation must initialize the AST type before the value escapes.  Report
snapshots must remain C-owned and must not allocate from the Lizard heap.

Do not disable this audit to land GC/concurrency work; update the ownership
model and tests intentionally when ownership rules change.

## GC metadata side table strictness

GC metadata must remain C-owned and off-object until the collector transition is
explicitly scheduled.  Metadata registration must not change allocation success
semantics: allocation can succeed even if metadata registration fails, but the
failure must be counted for diagnostics/audits.

## Build graph closure

Every implementation module in `src/*.c`, except `src/repl.c`, must be linked
into `liblizard.a`.  Tests must not depend on object files that are omitted from
the library archive.  Run:

```sh
make build-graph-audit
```

before changing source lists, syntax-object scaffolding, report modules, or GC
metadata modules.

## Conservative build graph closure

`liblizard.a` must include implementation files required by enabled headers and
tests, but it must not automatically compile every `src/*.c` file.  Some source
files are experimental or partial scaffolds and become hard errors under strict
flags if pulled into the build early.  Build graph closure is therefore an
allowlisted policy: add an implementation module to `LIB_OPTIONAL_SRCS` when it
is intentionally part of the library surface.

## Public API duplicate definitions

`include/lizard_api.h` must define each public struct/enum exactly once.  Patch
recovery scripts must normalize existing public API blocks before inserting
missing declarations.  `make api-audit` runs `scripts/check-public-api-duplicates.py`
to catch duplicate definitions such as repeated `lizard_expansion_trace_event_t`.

## Tokenizer/source wrapper strictness

If a header declares a non-static function, the corresponding implementation
must be present in the library archive. Do not leave scaffold prototypes without
definitions under the strict test build; they become link-time regressions.

Known-kind heap constructors should use `lizard_heap_alloc_tagged` instead of
relying on size inference when the object kind and trace policy are known.


## Context trace API visibility

Tests and embedders may use the context-level expansion trace API directly.
`include/lizard_api.h` must declare the trace controls and owned snapshot API;
otherwise strict builds fail with implicit declarations under `-Werror`.
