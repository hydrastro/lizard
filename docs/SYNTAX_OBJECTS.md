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
