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
