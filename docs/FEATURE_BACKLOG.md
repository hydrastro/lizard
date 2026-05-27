# Feasible feature backlog

This file divides the big ambition into buildable layers.

## Landed in this foundation patch

- Push-clean repository shape.
- `.gitignore` for generated artifacts.
- Fixed Makefile phony/build-directory conflict.
- `check` and `smoke` build targets.
- Stable opaque C API skeleton in `include/lizard_api.h`.
- Runtime/context façade for embedders.
- File/string evaluation API.
- Value classification and printing API.
- Token source-position metadata (`line`, `column`, `offset`).
- Tests for C API and token positions.
- CLI `--help`, `--version`, and direct file execution.
- Initial `lib/prelude.lisp` library location installed under `share/lizard`.

## Near-term, high leverage

1. Move all global state into `lizard_runtime_t`.
2. Add syntax objects carrying datum + lexical context + source span.
3. Replace direct parser specialization with reader -> expander -> core AST.
4. Add a module loader and `(module ...)` / `(import ...)` surface forms.
5. Add non-moving mark-sweep GC, then make API values proper handles.
6. Add bytecode IR with source-span side tables.
7. Add debugger hooks over bytecode frames.
8. Add profiler and coverage counters keyed by source span.

## Research tower

1. Separate a trusted type-checking kernel from exploratory reductions.
2. Add elaboration with holes/metavariables.
3. Add inductive families and termination/productivity checks.
4. Make CCHM primitives systematic: interval, face lattice, systems, comp,
   hcomp, fill, Glue, univalence computation.
5. Document every rule as checked, implemented-but-experimental, or notation.
