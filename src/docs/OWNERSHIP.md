# Runtime Value Ownership

This document records the current runtime ownership model and the Phase 3A
transition target.  It is intentionally conservative: the current allocator is
still segment-based and non-moving, while future work will move toward
object-level non-moving mark/sweep.

## Current model

### Heap-owned values

`lizard_heap_alloc` allocates values from the active Lizard heap segment.  These
objects are runtime-owned and currently reclaimed only by heap destruction or by
segment-level GC in `gc.c`.

Heap-owned values include:

- `lizard_ast_node_t` runtime values;
- `lizard_ast_list_node_t` wrappers;
- heap-resident copied strings used inside AST/runtime values;
- lists allocated through `list_create_alloc(lizard_heap_alloc, ...)`.

Heap-owned values must have stable addresses.  Moving them would break the C API,
FFI assumptions, existing environment bindings, and future concurrency rules.

### C-owned values

Some infrastructure objects are intentionally C-owned via `malloc`/`free`, not
Lizard heap-owned:

- runtime/context shells;
- REPL buffers/history;
- owned tooling reports;
- bytecode helper structures;
- persistent global registries that predate the object-level GC transition.

C-owned report objects must never allocate their snapshots through
`lizard_heap_alloc`, because reports are allowed to outlive an evaluation and
may be inspected after the context changes.

### Borrowed values

Borrowed pointers are never freed by the recipient.  Examples include public
report event views that expose `const char *` fields owned by the report object.
The caller must copy data if it needs a longer lifetime than the report.

## Constructor rule

Every direct `lizard_heap_alloc(sizeof(lizard_ast_node_t))` must initialize the
AST node type before the value escapes.  Prefer constructor helpers such as
`lizard_make_nil`, `lizard_make_bool`, and `lizard_make_error` when possible.
Direct AST allocation is still permitted in low-level expansion/evaluator code,
but it is audited.

## Phase 3A object model scaffold

`src/object_model.c` / `src/object_model.h` add a vocabulary for ownership and
future tracing policy:

- owner: heap, C malloc, borrowed, static, context;
- trace policy: none, AST, environment, list, custom;
- address stability and movability;
- explicit-free requirement.

This is metadata only.  It does not change allocator layout or collection
behavior yet.

## Future transition target

The next memory-system goal is object-level non-moving mark/sweep:

1. introduce per-object metadata/header or side metadata;
2. keep object addresses stable;
3. trace heap-owned AST/list/env objects precisely;
4. keep C-owned reports/runtime shells outside the GC heap;
5. add safepoints before real multithreaded collection.

The current `ownership-audit` target exists to keep the tree ready for that
transition.

## Phase 3B: GC metadata side table

Phase 3B adds a per-heap GC metadata side table.  The table is C-owned and is
stored off-object, so heap object layout and addresses remain unchanged.  This
is intentionally a scaffold for future object-level non-moving mark/sweep, not
a collector rewrite.

The side table records, for each registered heap allocation:

```text
pointer
size
object kind
owner
trace policy
```

The current heap allocator registers allocations opportunistically.  Failure to
record metadata increments side-table failure accounting but does not make
runtime allocation fail; this preserves evaluator behavior while making GC
metadata visible for audits and future debugging.

Object kinds introduced in this phase:

```text
unknown
ast-node
ast-list-node
env
env-entry
kernel-term
raw
```

The table is explicitly non-moving: it describes objects but never relocates or
rewrites pointers.

## Phase 3C build-graph closure note

Ownership and GC metadata code is only useful if its implementation object is
actually linked into `liblizard.a`.  The build graph audit now enforces that all
`src/*.c` implementation modules except the REPL entry point participate in the
library archive.  This is especially important for GC metadata and syntax/report
scaffold modules, which are easy to add as headers/tests before wiring into the
archive.

## Phase 3D build recovery note

The GC/ownership scaffolding is only useful if the library build graph contains
its intended implementation files and excludes incomplete experiments.  Phase 3D
keeps ownership/runtime modules explicit and optional rather than compiling all
of `src/*.c` indiscriminately.

## Phase 3F: explicit allocation classification

The heap now exposes `lizard_heap_alloc_tagged(size, kind, trace_policy)` for
constructor paths that know the object kind at allocation time. The older
`lizard_heap_alloc(size)` remains available and still performs conservative
size-based classification, but core constructors should prefer tagged
allocation when they know the object kind.

This remains behavior-preserving: the collector is still non-moving and does
not yet use object metadata to sweep individual objects.

