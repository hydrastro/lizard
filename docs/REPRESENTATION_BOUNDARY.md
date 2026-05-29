
## Phase 2Z: unified diagnostic construction

Diagnostics now have one construction path in `diagnostics.c`.  Tokenizer,
parser, runtime, syntax-expansion, and report code should construct diagnostics
through the same helper functions so status, severity, category, source span,
and message truncation behavior remain consistent across the tooling boundary.

## Phase 3A: ownership boundary

Runtime values, C-owned tooling reports, borrowed event views, and future
GC-traced objects now have an explicit ownership vocabulary in
`object_model.c`.  The vocabulary does not change semantics yet; it makes the
value/runtime boundary auditable before object-level GC work begins.


## Phase 3B: off-object GC metadata

The runtime now has an off-object GC metadata side table.  It records object
kind, size, ownership, and trace policy while preserving the value
representation boundary: runtime objects are not moved, resized, or extended in
this phase.  This gives the future object-level collector observable metadata
without changing evaluator semantics.

## Phase 3F: explicit runtime object classification

Runtime constructors can now register object metadata explicitly at allocation
time. This keeps the value representation boundary observable for future
object-level mark/sweep without changing object layout, moving objects, or
changing evaluator behavior.

