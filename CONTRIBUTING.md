# Contributing to Lizard

Lizard is a C89 Scheme with a small trusted dependent-type-theory kernel and a
large library tower built on top of it. The guiding principle is **a small
trusted core and a large derived tower**: C is responsible for runtime,
representation, evaluation, memory, and the proof kernel; everything else is a
Lisp library that cannot compromise soundness. Contributions are welcome, and
this document describes the workflow that keeps the tower sound and the build
green.

## Building and testing

```
make                # build; binary lands at build/lizard
make test           # unit tests (the tests/ golden files)
make examples       # run every example, checked against examples/manifest.sexp
make check          # test + examples
```

The build depends on the `ds` data-structures library. If you see missing
headers, ensure the include and library paths point at it before building, e.g.

```
export C_INCLUDE_PATH=/path/to/ds LIBRARY_PATH=/path/to/ds LD_LIBRARY_PATH=/path/to/ds
make build/lizard
```

`make examples` is honest: it fails if any example does not match its recorded
behavior in `examples/manifest.sexp`. The full suite is large; while iterating
on one library you can run a single example or test directly:

```
build/lizard < examples/NNN-name.lisp
build/lizard < tests/cas_name.lisp
```

## How a library module is structured

Library code lives under `lib/`, with the computer-algebra tower in `lib/cas/`.
A module is imported by path starting at `lib/`, so `(import "cas/poly.lisp")`
resolves `lib/cas/poly.lisp`. Each module begins with a header comment stating
what it does, the mathematics it relies on, its public functions, and what it
verifies, followed by its imports and definitions.

Some conventions the interpreter enforces, worth knowing before you write Lisp
here:

- `if` takes exactly three arguments; use `cond` for multi-way branches.
- `max` and `min` are binary.
- Prefer top-level prefixed helper functions over internal `define`s and over
  deeply nested `let` forms; flatten nested `let`s into named helpers.
- `member`, `assoc`, and friends are not all built in; small modules define
  their own list helpers (see the `ps2-`, `e3r-`, `sup-` prefixes for the
  pattern).
- Polynomials are coefficient lists low-to-high: `(1 0 2)` is `1 + 2x^2`.

## The per-feature workflow

Every new capability follows the same disciplined path. This is what keeps the
tower sound and regression-free.

1. **Verify the mathematics first.** Before writing module code, confirm the
   algorithm by hand on concrete inputs (a scratch script under `/tmp` is fine).
   The strongest validation is two independent methods agreeing.

2. **Check for collisions before creating a module.** Run an additive-safety
   grep for the module name and your function prefix across `lib/`, `examples/`,
   and `tests/`:

   ```
   grep -rln "mymodule" lib/ examples/ tests/
   grep -rln "define (myprefix-" lib/
   ```

   A leaf module imported only by its own example and golden cannot affect any
   existing result. Prefer dropping a redundant module over duplicating one.

3. **Write the module** under `lib/cas/` (or the appropriate `lib/` subtree)
   with a full header comment and a unique function prefix. Keep the trusted
   kernel untouched.

4. **Test every case, including the honest-failure controls.** A sound module
   never returns a guessed answer: it returns a definite "don't know" verdict
   (for example `no-solution`, `irrational-fiber`, `notrecognized`) when it
   cannot decide. Test those paths too.

5. **Add an example** `examples/NNN-name.lisp` that demonstrates and checks the
   feature, guarding each assertion so a failure raises. Register it in
   `examples/manifest.sexp` and validate with
   `bash scripts/check-example-manifest.sh`.

6. **Add a golden test** `tests/cas_name.lisp` whose raw stdout is recorded in
   `tests/cas_name.expected`. Generate it once, then re-run and `diff` to
   confirm determinism.

7. **Re-run golden integrity** across the modules you touched and their
   dependencies; expect zero regressions.

8. **Document** the new capability with a short prose section in
   `docs/CAS.md`, and update `LIMITATIONS.md` / `docs/CLAIMS_MATRIX.md` if the
   scope of what works has changed.

## Certificates and honest scope

The project's distinguishing discipline is that **every positive result carries
a machine-checkable certificate** — a differentiation check for an integral, a
reduction modulo a Groebner basis for ideal membership, an exact evaluation to
zero for a solution tuple, or two independent computations agreeing. When a
problem is genuinely open, the code says so precisely rather than overclaiming.
Please preserve this: do not add a code path that returns an uncertified
"elementary" / "solved" answer, and do not weaken an honest "don't know" into a
guess. `LIMITATIONS.md` records the candid scope and should stay accurate.

## Audits

Several `scripts/check-*.sh` and `scripts/check-*.py` audits guard structural
invariants (header/include boundaries, ownership, the build graph, public-API
duplicates). Run the relevant ones for the area you changed; they are fast and
catch layering mistakes early.

## Submitting

Keep changes additive where possible, with the example and golden that prove the
behavior. A good change is one a reviewer can verify by running `make check` and
reading the new example.
