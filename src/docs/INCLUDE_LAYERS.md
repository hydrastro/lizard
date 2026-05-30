# Include Layering

Lizard's C headers are now treated as an architecture boundary, not just a
compiler convenience.  The goal is to prevent accidental coupling while the
runtime, syntax-object infrastructure, report tooling, and type-theory kernel
continue to grow.

## Layers

```text
include/lizard_api.h
  Stable public C API.  May include only standard C headers.

include/lizard.h
  Compatibility/public wrapper.  May include only lizard_api.h.

src/lizard_internal.h
  Implementation root.  May include public API and external implementation
  dependencies such as ds/gmp/system headers.

src/* report/tooling leaf headers
  Small internal headers such as report_writer.h, diagnostic_report.h, and
  syntax_expansion_report.h.  They must not pull in the whole interpreter core
  unless they truly need it.

src/* implementation headers
  Runtime/parser/tokenizer/kernel/type-theory internals.  These may include
  other src headers, but the implementation-header graph must stay acyclic.
```

## Audits

Run:

```sh
make header-audit
make include-audit
```

`header-audit` checks public API type visibility: if an implementation header
publishes a type from `lizard_api.h`, it must include `lizard_api.h` itself.
That prevents include-order bugs such as a header exposing
`lizard_expansion_trace_event_t` without making the type visible.

`include-audit` checks architectural layering:

- public headers do not include implementation headers;
- implementation headers do not include the public wrapper `lizard.h`;
- parent-relative includes are rejected;
- selected leaf/tooling headers do not depend on the large interpreter core;
- the quoted include graph among `src/*.h` is acyclic.

## Policy

Do not fix include failures by relying on transitive includes.  A header must
include the headers required by the declarations it exposes.  This keeps strict
builds deterministic and makes the future module/macro/runtime split easier.
