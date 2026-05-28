# Lizard v5 — engineering infrastructure (in progress)

This section logs the major infrastructure work built on top of the v4
type-theory + diagnostics baseline.

## Phase 0: Runtime refactor (B.1–B.2)

- Added `lizard_runtime_t *runtime` back-pointer on `lizard_heap_t`, so
  any function holding the heap can reach runtime state via `heap->runtime`.
- Moved 9 process-global variables into `lizard_runtime_t`:
  `gensym_counter`, `sr_counter`, `callcc_buf/active/value`,
  `logic_config_head`, `logic_last_set_bundle`, `hit_registry_head`,
  `flag_list`.
- Accessor functions (`logic_config_ptr()`, etc.) with static fallbacks
  for backward compat with standalone heaps.
- Sequential multi-instance now works: two runtimes in the same process
  have independent logic configs, HIT registries, and counters.

## Phase C: Module loader

- `(import "path.lisp")` — load once with caching, resolved via search path.
- `(module-loaded? "path")`, `(module-search-path)`, `(add-module-path! "dir")`.
- Default search path includes `lib/`. Module cache keyed by both raw
  and resolved paths.

## Phase D: Garbage collector (D.1–D.3)

- `gc_mark` bit on every `lizard_ast_node_t`. `lizard_heap_alloc` zeroes
  memory to ensure marks start at 0.
- `lizard_gc_mark_node` — recursive mark traversal covering 80+ AST types.
- `lizard_gc_mark_env` — walks environment chains and closure captures.
- Segment-level sweep: `lizard_gc_collect` frees heap segments with zero
  live objects. No pointer updating needed.
- `(gc)` — run mark + sweep, report freed bytes and before/after stats.
- `(gc-stats)` — report segments, bytes, total/live/garbage node counts.

## Phase E: Bytecode compiler + VM (E.1–E.2)

- 30-opcode stack-based VM (`src/bytecode.h`, `src/bytecode.c`).
- Compiler handles: constants, variables, arithmetic, comparisons,
  if/else, define/set!, lambda with closures, general function calls,
  begin, cons/car/cdr, display/newline.
- **Tail-call optimization**: `OP_TAIL_CALL` replaces the current frame
  (chunk, env, ip, sp) and restarts the dispatch loop. Zero C stack growth.
- `(vm-eval expr)`, `(disassemble expr)`, `(vm-time expr)`, `(time-eval expr)`.

## Profiler

- `(profile expr)` — compile + execute with full instruction counting.
  Reports: elapsed time, total instructions, call/tail-call counts,
  MIPS, and per-opcode breakdown.

## Structured diagnostics (Phase F)

- `lizard_make_error_at(heap, code, span)` — error with source location.
- Six key eval error paths carry spans (unbound symbol, invalid apply,
  bad define, bad assignment).
- Error printer prepends `line:col:` when span is available.
- `(error-location err)` — programmatic access to error span.

## Documentation

- `docs/HIT.md` — comprehensive HIT layer reference (~350 lines).
- `docs/MODAL.md` — updated with full M.5.* coverage.
- `DESIGN.md` — added "Further documentation" index.
- `docs/CLAIMS_MATRIX.md` — updated throughout.

---

# Lizard v4 — type-theory expansion + diagnostics/scaffolds (in progress)

This section logs the changes from v3 (post-restructure baseline) to the current
head. The v3 section follows. For per-phase detail, see `DESIGN.md`, `docs/MODAL.md`,
`docs/CLAIMS_MATRIX.md`, and `docs/OPTIONAL_PROOF_SCAFFOLDS.md`.

## Public/internal header split

- `include/lizard.h` is now a 13-line compatibility shim re-exporting
  `include/lizard_api.h`. All AST node definitions and interpreter internals
  moved to `src/lizard_internal.h`.
- Embedders get a stable opaque public surface. Internals can evolve without
  ABI breaks. Older embedders that `#include <lizard.h>` keep working.

## Source spans on every AST node

- `lizard_source_span_t span` field on each `lizard_ast_node_t`.
- Tokens already carried `line`/`column`/`offset`; this propagates them
  forward into the AST so diagnostics can point at the right place in source.
- Foundation for future structured error messages.

## Scaffold/checked discipline

- New convention: experimental syntax lives behind opt-in **logic rules** and
  is documented as "scaffold" in `docs/CLAIMS_MATRIX.md` until promoted to
  "checked" status.
- New bundles: `cubical-S1`, `truncations`, `proof-scaffold`.
- New toggles: `cubical-s1-enabled`, `truncations-enabled`,
  `theory-extensions-enabled`.
- Documented migration path in `docs/OPTIONAL_PROOF_SCAFFOLDS.md`.

## Phase H.2 — Propositional truncation promoted from scaffold to checked

- AST nodes `Trunc`, `trunc`, `trunc-elim` (originally scaffold) now have real
  typing rules and a primary computation rule.
- Typing:
  - `(Trunc level A) : Universe-of-A` — universe-preserving.
  - `(trunc x) : (Trunc A)` infers `A` from `x : A`; level left NULL.
  - `(trunc-elim C h e) : C` when `e : (Trunc _ A)` and `h : Π _:A. C`.
- Reduction: `(trunc-elim C h (trunc x)) ⟶ (@ h x)`, deterministic.
- Honest gap: propositionality obligation on motive `C` not structurally
  enforced (see `docs/CLAIMS_MATRIX.md`).
- All operations gated on `truncations-enabled`.
- New test `tests/tt_truncation_test.c`, walkthrough `examples/62-truncation.lisp`.

## Cubical S¹ scaffold (unchanged from upload)

- `S1`, `base`, `loop` with minimal typing spine.
- Remains scaffold-only — no recursor, no `loop`-computation rule, no
  Kan composition.

## Type-theory work prior to H.2 (carried forward from earlier v4 state)

- Lambda cube (M.2, M.3): 8 cube corners + CoC as named bundles.
- Substructural rules (M.4): `weakening`/`contraction`/`exchange` toggles.
- Universe lattice (L.1–L.5): pi-fresh/co-pi-fresh, couniverse, lattice toggles.
- HIT scaffolding (H.1): AST nodes + registry, no computation rules.
- Modal logic layer (M.5.1–M.5.9): K, T, S4, S5 operationally distinct;
  asymmetric forms (box/unbox/diamond/let-diamond/box-app/diamond-bind);
  **symmetric S5 (M.5.9 Turn 2b)**: `dia`, `poss-coerce`, judgment-kind
  tracking, kind propagation through `let-diamond`, kind check in `box-intro`.
  See `docs/MODAL.md`.

## Diagnostics and proof-scaffold infrastructure (community contribution)

- `docs/CLAIMS_MATRIX.md` — precise feature status (implemented / partial /
  scaffold / not implemented), updated whenever a feature changes tier.
- `docs/OPTIONAL_PROOF_SCAFFOLDS.md` — explains the scaffold/checked
  discipline and the intended migration path.
- `tests/tt_optional_extensions.lisp` + `.expected` — golden test for the
  new opt-in nodes at the construction layer.
- Enhanced `runtime.c/h` and `lizard_api.h` for richer embedding surface.
- Generic `theory-extension` AST node (scaffold) for plugging in
  experiments without changing the AST.

## Scoreboard at v4 head (after H.2 + merge)

```
57 C unit tests + 5 Lisp golden tests passing
62+ examples including modal layer (M.5.*) walkthroughs and H.2 truncation
Benchmark: ~0.5s
Builds clean: release, debug, asan, coverage
```

---

# Lizard v3 — restructure + tests + features

## Structural changes

- **`src/` + `include/` split.** All implementation in `src/`, single
  public header `include/lizard.h`. The public header no longer
  pulls in internal `errors.h`/`en.h`/`lang.h`; those become true
  internals and are included only by the `.c` files that need them.
- **`tests/` directory** with:
  - A header-only test harness (`test_harness.h`).
  - A façade (`test_helpers.{h,c}`) that gives each test one line to
    spin up a fresh interpreter and another to evaluate strings.
  - Five C unit tests (`arith_test`, `control_test`, `lambda_test`,
    `lists_test`, `macros_test`).
  - Two golden-output tests (`scripting`, `error_propagation`) with
    matching `.expected` files.
- **`examples/`** unchanged in content but now formally part of the
  layout (with a README of its own).
- **`tests/tests.mk`** included from the top-level Makefile gives
  `make test`, `make test-c`, `make test-lisp`, plus colourised
  PASS/FAIL output per test.
- **`lizard_install_primitives()`** moved from `repl.c` into the
  library so tests and the REPL share one source of truth for the
  set of registered built-ins.

## New features

- **`display`**, **`write`**, **`newline`** primitives — standard
  Scheme I/O so `.lisp` scripts can produce real output rather than
  relying on the REPL's auto-echo.
- **`(load "path")`** primitive — reads a file and evaluates every
  top-level form in the current environment. Reports proper errors
  (`LIZARD_ERROR_LOAD_ARGC`, `_ARGT`, `_OPEN`, `_READ`). Lets
  scripts depend on the prelude:
  ```lisp
  (load "examples/prelude.lisp")
  (display (sum (range 1 101))) (newline)   ; 5050
  ```

## Bugs fixed since the user's snapshot

The upload had the begin/macro fixes but reverted several others. This
release re-applies them on top of the upload's structural improvements
(typedef aliases, `lizard_make_number_copy`, `static` globals,
`lizard_repl_strdup`):

1. **Tokenizer treats all whitespace as whitespace.** `\t`, `\n`, `\r`
   are skipped; previously only `' '` was, so any multi-line file with
   tabs or newlines tokenised into garbage symbols.
2. **`;` line comments** are recognised by the tokenizer.
3. **REPL is stdin-friendly.** When stdin is not a TTY the prompt,
   raw-mode line editor, and `\033[K` escapes are suppressed. The
   continuation join uses `\n` instead of a space so `;` comments
   terminate at the original line boundary.
4. **Scheme-style value printer in the REPL.** Output is `3628800` /
   `(1 2 3)` / `<procedure>`, not `Number: 3628800` and 14-line AST
   dumps. Errors print only their message (no doubled `error: Error:`).
5. **`cond` is implemented in the evaluator.** The parser had been
   building `AST_COND` nodes, but the evaluator had no case for them,
   so every `cond` form returned `LIZARD_ERROR_NODE_TYPE`.
6. **Multi-body function definitions** (`(define (f x) e1 e2 e3)`)
   parse correctly; previously only the first body expression was
   consumed and the rest produced "missing closing paren in define".
7. **`let` accepts non-symbol binding names in macro bodies**, so
   `` `(let ((,name ,value)) ,body) `` works.
8. **Unquote-splicing handles real cons-pair lists**, not only
   `AST_APPLICATION`. Empty list splices to nothing.
9. **Zero-parameter lambdas/macros work.** `((lambda () 42))` and
   `(define-syntax k (lambda () 42))` no longer report "alternative
   lambda parameter format is wrong" — the parser turns `()` into
   `AST_NIL`, which both call paths now accept.
10. **Unbound-symbol errors propagate** instead of being rewritten to
    "attempt to apply a non-function" when they appear in the operator
    position.

## What's verified

`make test`:

```
  PASS  arith_test          (mutation guard, variadic +/-/*//, bignums, div0)
  PASS  control_test        (if, cond/else, begin, and/or/not, let)
  PASS  lambda_test         (recursion, multi-body, mutual, bignum, closures)
  PASS  lists_test          (cons, car/cdr, quote, predicates, user map)
  PASS  macros_test         (quasiquote, splice, special-form expansion, let)
C tests: 5/5 passed
  PASS  error_propagation   (golden-output)
  PASS  scripting           (golden-output)
Lisp tests: 2/2 passed
All tests passed.
```

`make examples` runs all six example files and the prelude without
errors. `06-scripting.lisp` produces a formatted factorial table
ending in `20! is exactly: 2432902008176640000`.

## Open

- `call/cc` still uses module globals (`callcc_buf`, `callcc_active`);
  nested `call/cc` will clobber itself. No regression test for it
  because the existing implementation isn't reliable enough to pin
  golden behaviour to.
- `write` is currently identical to `display` for non-string values.
  In R5RS Scheme, `write` escapes strings (`"a\"b"` would print
  literally). Worth tightening.
- No `string-length`, `string-append`, `number->string` yet — the
  string facilities are minimal. Add when needed by a script.
- Memory: `lizard_heap_destroy` releases everything at REPL exit,
  but inside a long session the bump arena grows and never returns
  memory to the OS (it's grow-only, not a real GC).

# Lizard v4 — performance round

## Tail-call optimisation

Lambda application now tail-calls its **last body expression** by
trampolining through the existing `for(;;)` dispatch loop in
`lizard_eval` — the same mechanism that already drove TCO for `if`,
`begin`, and `cond`. Non-tail bodies are still evaluated for side
effects only, then `(node, env)` are rewritten to the tail body and
`continue` is used.

Before this change every Scheme call ate a real C stack frame:
```lisp
(define (count n) (if (= n 0) 'done (count (- n 1))))
(count 100000)    ; segfault — 8MB C stack exhausted around ~10^4
```

After this change a properly tail-recursive function runs to any
depth limited only by the heap. The new `tco_test.c` exercises
100,000 iterations of plain tail recursion, 50,000 of mutual tail
recursion (`even?`/`odd?`), tail calls out of `cond` clauses, and
tail calls as the last expression of a `begin`.

## Fast bignum primitives

Lizard's bump-allocator heap is grow-only and has no GC, so
loops like
```lisp
(define (pow2-iter n acc) (if (= n 0) acc (pow2-iter (- n 1) (* 2 acc))))
(pow2-iter 1000000 1)
```
allocate `O(N²)` of intermediate bignums and exhaust memory long
before they finish — even with TCO eliminating the stack cost.
MIT Scheme handles this because of its generational GC; lizard
now sidesteps the problem entirely by exposing GMP's fast routines
as primitives so the same computation completes in *one* mpz call:

| Primitive            | Backed by         | Cost     |
| -------------------- | ----------------- | -------- |
| `arithmetic-shift`   | `mpz_mul_2exp` / `mpz_fdiv_q_2exp` | O(1) GMP |
| `expt`               | `mpz_pow_ui`      | O(log e) GMP |
| `gcd`                | `mpz_gcd`         | Lehmer in GMP |
| `lcm`                | `mpz_lcm`         | Lehmer + multiply |
| `quotient`           | `mpz_tdiv_q`      | one division |
| `remainder`          | `mpz_tdiv_r`      | one division |
| `abs`                | `mpz_abs`         | one op |
| `square`             | `mpz_mul`         | one op |
| `modular-expt`       | `mpz_powm`        | O(log e), bounded intermediates |

`(arithmetic-shift 1 1000000)` now produces a 301,030-digit exact
integer in milliseconds. `(modular-expt 2 65537 (- (expt 2 127) 1))`
finishes instantly — the RSA-style core works on arbitrary sizes.

## New tests

- `tests/tco_test.c` — depth-100,000 tail recursion, mutual
  recursion, `cond` and `begin` tail positions.
- `tests/fastprims_test.c` — arithmetic-shift (left/right/huge),
  expt, gcd, lcm, quotient/remainder (with negative dividends),
  abs, square, modular-expt, plus the error paths
  (`(quotient 1 0)`, `(expt 2 -1)`, type mismatch).

## New example

- `examples/14-perf.lisp` — count-down from 1,000,000;
  `arithmetic-shift 1 1000000`; `expt 7 5000`; gcd of two giant
  bignums; a toy RSA encrypt/decrypt cycle.

## What's still not addressed

The arena is still grow-only. Long-running idiomatic Scheme
without the fast primitives still piles up garbage. A real GC
is the right next investment — generational, with the arena
becoming the nursery. That's a larger surgery and was deliberately
kept out of this round so the TCO + fast-prims wins land cleanly.

# Lizard v5 — reflection, types, strings

## Type reflection

```scheme
(type-of 42)           ; 'number
(type-of "hi")         ; 'string
(type-of 'foo)         ; 'symbol
(type-of '(1 2))       ; 'pair
(type-of (lambda(x)x)) ; 'procedure
(type-of +)            ; 'primitive
(type-of #t)           ; 'boolean
```

Returns a symbol naming the runtime type. Covers every AST node
type lizard can produce: number, string, symbol, boolean, nil,
pair, list, procedure, primitive, macro, quote, quasiquote,
promise, error, continuation.

## Environment + procedure introspection

```scheme
(defined? 'my-binding)       ; #t / #f without throwing
(env-keys)                   ; list of every bound symbol in scope
(procedure-arity f)          ; number of formal params, or 'variadic
```

`env-keys` walks every frame from innermost outward; primitives,
user-defined functions, and macros all appear. `procedure-arity`
returns a number for lambdas (counting positional params, including
0 for `(lambda () …)`) and the symbol `'variadic` for C primitives.

## Strings — proper operations, not just opaque blobs

```scheme
(string-length s)
(string-append a b ...)          ; n-ary
(substring s start [end])        ; end defaults to length
(string=? a b)
(number->string n)
(string->number s)               ; -> number, or #f if unparseable
(symbol->string s)
(string->symbol s)
```

The tokenizer also now handles backslash escapes inside string
literals: `\n` `\t` `\r` `\\` `\"` `\0`. Before this change `"say
\"hi\""` lex-failed; now it parses to the obvious 9-char string.

## Records, the lisp-y way

No new C — records are tagged conses with a small Lisp protocol:

```scheme
(define (make-point x y) (make-record 'point (list x y)))
(define (point-x p) (field-nth p 0))
(define (point-y p) (field-nth p 1))
(record? (make-point 3 4))     ; #t
(record-type (make-point 3 4)) ; 'point
```

See `examples/15-types.lisp` for the full pattern. Build on
`record?`, `record-type`, and positional `field-nth` to define any
record shape you want.

## Pattern dispatch

Lizard doesn't have varargs in lambdas, so there's no `syntax-rules`
shaped `match` macro yet. `examples/16-match.lisp` shows a runtime
dispatcher that takes a list of `(tag thunk)` clauses and threads
into the matching one. The example builds a tiny arithmetic
AST, an evaluator, and full symbolic differentiation on top of it:

```
d/dx (3 + 4*x) = (plus (lit 0) (plus (times (lit 0) (var x))
                                     (times (lit 4) (lit 1))))
d/dx -(x + 7) evaluates to -1
```

## New tests

- `tests/reflection_test.c` — every type-of case, defined? for
  bound/unbound/non-symbol args, env-keys completeness via
  member-search, procedure-arity for 0/1/N parameters and
  primitives.
- `tests/strings_test.c` — length/append/substring/equality,
  number↔string round-trip, symbol↔string round-trip, bignum→string,
  out-of-range substring errors.

## New examples

- `examples/15-types.lisp` — reflection, records, an `inspect`
  function that dispatches on `type-of` and prints type-aware
  summaries.
- `examples/16-match.lisp` — pattern dispatcher + symbolic
  differentiation of `(3 + 4x)`, `x*x`, `-(x+7)`.
- `examples/17-strings.lisp` — string manipulation, conversions,
  string-reverse via recursive substring, formatted output helper.

## Scoreboard

```
$ make test
15/15 C tests passing  (arith, bignum, closures, control,
                        deep_recursion, errors, fastprims,
                        higher_order, lambda, lists, macros,
                        quasiquote, reflection, strings, tco)
4/4  Lisp golden tests passing
All tests passed.
```

# Lizard v6 — varargs, exceptions, vectors, hashes (extreme edition)

This release adds four major language features and tightens error
semantics so they all compose cleanly.

## Varargs lambdas

```scheme
(define sum (lambda xs
  (define (loop ys acc)
    (if (null? ys) acc (loop (cdr ys) (+ acc (car ys)))))
  (loop xs 0)))

(sum)                  ; 0
(sum 1 2 3 4 5)        ; 15
(sum 1 2 3 4 5 6 7 8 9 10)  ; 55
```

When a `lambda`'s parameter spec is a single symbol instead of a list,
the symbol is bound to a list of *all* the call arguments. This is the
R5RS "rest" form and unlocks `(define (count . xs) ...)`-style
patterns. Implemented at both eval-time and `lizard_apply` call sites.

Dotted-pair varargs `(lambda (a b . rest) ...)` is not yet supported —
that would need new tokenizer support for `.`.

## Exceptions: try / raise / error-object? / error-value

```scheme
(define (safe-div a b)
  (try (lambda () (/ a b))
       (lambda (err)
         (display "caught: ") (display (error-value err)) (newline)
         0)))

(safe-div 10 2)   ; 5
(safe-div  7 0)   ; prints diagnostic, returns 0
```

User code can raise structured payloads:

```scheme
(raise (list 'invalid-input value "must be positive"))
```

The handler receives an error object and uses `(error-value err)` to
unwrap the payload. `error-object?` tests whether a value is an error.

Two C primitives are exempt from auto-propagation (see below) so that
they can *receive* errors as values: `error-object?` and `error-value`.

## Vectors

```scheme
(define v (vector 10 20 30 40 50))
(vector-length v)         ; 5
(vector-ref v 2)          ; 30
(vector-set! v 2 999)
v                         ; #(10 20 999 40 50)
(vector->list v)          ; (10 20 999 40 50)
(list->vector '(a b c))   ; #(a b c)
```

O(1) indexed access + mutation. Fixed-size; create with
`(make-vector n [fill])` or `(vector v1 v2 ...)`. Printer renders as
`#(...)`. `vector?` predicate.

## Hash tables

```scheme
(define h (make-hash-table))
(hash-set! h 'name "lizard")
(hash-set! h 'age 5)
(hash-ref h 'name)              ; "lizard"
(hash-ref h 'missing 'default)  ; default
(hash-has-key? h 'name)         ; #t
(hash-size h)                   ; 2
(hash-keys h)                   ; (age name)
(hash-remove! h 'age)
```

Open-addressed with linear probing. Doubles capacity when load > 0.75.
Hash function: FNV-1a for strings/symbols, mpz_get_ui for numbers.
Key equality is value-based for numbers, strings, symbols, booleans,
and nil. Printer renders as `#hash((k . v) ...)`.

## Auto-propagation of errors

Before this release, `(display (raise 'oops))` would silently print
`oops` because display didn't know its arg was an error. Now errors
short-circuit at every eval boundary:

- **Primitive calls**: if any argument forces to AST_ERROR, the call is
  skipped and the error propagates (except for `error-object?` and
  `error-value`, which receive errors as data).
- **`if`, `cond`**: an erroring predicate propagates without firing
  any branch.
- **`begin`, lambda body**: an intermediate expression that errors
  short-circuits the sequence.

This makes `(try ...)` actually work the way users expect — without
this, a `(raise ...)` inside a `display` call would be silently
absorbed.

## gensym

```scheme
(gensym)              ; g1
(gensym)              ; g2
(gensym "tmp-")       ; tmp-3
```

Produces a fresh symbol on each call. Useful for hand-written
hygienic-ish macros until proper `syntax-rules` lands.

## type-of extended

Now covers vectors and hashes:

```scheme
(type-of (vector 1 2 3))     ; vector
(type-of (make-hash-table))  ; hash
```

## String escape sequences (bonus)

The tokenizer now decodes `\n`, `\t`, `\r`, `\\`, `\"`, `\0` inside
string literals. Before, `"say \"hi\""` lex-failed.

## New tests

- `tests/exceptions_test.c` — raise + try + nested try + error-value
  with various payload types + system errors (div by zero)
- `tests/vectors_test.c` — every op + mutation persistence + accumulator
  via vector + mixed-type contents + out-of-range errors
- `tests/hashes_test.c` — 200-entry growth stress + key types
  (symbols, strings, bignums) + remove + missing-key + default
- `tests/varargs_test.c` — zero/one/many args + mixed types + variadic
  sum, max, count

## New example

`examples/18-extreme.lisp` exercises all four new features in one
program:
- variadic `puts` formatter dispatching on `type-of`
- in-place insertion-sort on a vector
- word-frequency counter on a hash table
- `safe-div` with try/raise/error-value
- structured `(invalid-age N reason)` payloads with cond-based handler
- fresh gensyms

```
$ ./build/lizard < examples/18-extreme.lisp
hello world, 2025
type-of 42 is: number

before sort: #(5 2 8 1 9 3 7 4 6)
after sort:  #(1 2 3 4 5 6 7 8 9)

word frequencies: #hash((brown . 1) (lazy . 2) (dog . 1) ...)
  'the' appeared 4 times
  'fox' appeared 2 times
  'lazy' appeared 2 times

safe-div 10 2 = 5
  caught error, falling back to 0
safe-div  7 0 = 0
safe-div 99 3 = 33

valid age: 25
valid age: rejected age -5: must be non-negative
valid age: rejected age 200: too old to be plausible
valid age: 99

fresh gensym: swap-tmp-1
another:      swap-tmp-2
distinct?     #t
```

## Scoreboard

```
$ make test
19/19 C tests passing  (arith, bignum, closures, control,
                        deep_recursion, errors, exceptions,
                        fastprims, hashes, higher_order, lambda,
                        lists, macros, quasiquote, reflection,
                        strings, tco, varargs, vectors)
4/4  Lisp golden tests passing
All tests passed.
```

## Known deferred work

- **Hygienic macros (`syntax-rules`)** — would need proper
  alpha-renaming. Significant.
- **Dotted-pair varargs `(lambda (a b . rest) body)`** — tokenizer
  needs `.` handling.
- **Real GC** — currently grow-only arena. A generational collector
  with the arena as the nursery is the obvious next step.
- **call/cc** — exists but uses module globals (`callcc_buf`,
  `callcc_active`), so nested call/cc would collide.
- **`write` vs `display` on strings** — currently identical; R5RS
  says write should escape.

## Phase 3: Deep feature expansion

### Kernel completion
- Sigma types: full inference + computation (proj1/proj2 reduce on pairs).
- J-eliminator: `J C d A a b (refl a) → d a` — the fundamental identity
  elimination principle.
- `sexp_to_kterm` extended: `natrec`, `app`, `lam`, `Pi`, `Sigma`, `pair`,
  `fst`, `snd`, `J`, `Id`, `refl` — the full dependent type theory toolkit.
- `(kernel-reduce expr)` — normalize kernel terms to WHNF.
- `(kernel-equal? a b)` — check definitional equality.

### Proof state + tactics
- `src/tactics.c` + `src/tactics.h` — proof state management.
- `(begin-proof type)` — start proof with one goal.
- `(tactic-intro name)` — introduce Pi binder.
- `(tactic-exact term)` — provide exact proof term.
- `(tactic-refl)` — solve Id a a goals.
- `(qed)` — finish proof, extract term.

### Track C: Atoms
- `(atom val)`, `(deref a)`, `(swap! a f)`, `(reset! a v)`, `(atom? x)`.
- Mutable reference cells with CAS-style swap.

### Exceptions
- `(raise val)` — raise an exception.
- `(guard handler body)` — catch exceptions.

### String operations
- `string-ref`, `string-contains?`, `string-upcase`, `string-downcase`.
- `string-split`, `string-join`.

### Lazy evaluation
- `(delay expr)` — create a promise (thunk).
- `(force p)` — evaluate and cache.
- `(promise? x)` — predicate.

### Standard library (lib/match.lisp)
- sort, zip, partition, flatten, take, drop, any, every, enumerate.
- compose, ->, alist-ref, alist-set, range.
- identity, const, flip, curry, uncurry, repeat, complement, memoize, juxt.

### Persistent data structures
- Persistent vectors: pvec, pvec-ref, pvec-set, pvec-push, pvec-count.
- Persistent hash maps: phash-map, phash-get, phash-set, phash-keys.
- Syntax objects: datum->syntax, syntax->datum, syntax-e, syntax-source.

## Phase 4: Elaboration foundations

### Metavariables / Holes (K.4)
- `KT_META` kernel term tag — placeholder for unknown terms.
- `meta_ctx_t` — metavariable context tracking type, solution, and ID.
- `meta_fresh(heap, mctx, type)` — create a fresh typed hole.
- `meta_solve(mctx, id, solution)` — fill a hole.
- `meta_zonk(heap, mctx, term)` — substitute solved metas in a term.
- `(kernel-hole type)`, `(kernel-solve id term)`, `(kernel-zonk term)`,
  `(kernel-meta-state)` — Lisp-facing primitives.

### Unification (K.5)
- `kt_unify(heap, ctx, mctx, a, b)` — first-order unification.
- Flex-rigid: unsolved meta on one side → solve with the other.
- Structural: recursively unify corresponding subterms.
- Works with WHNF reduction — unfolds definitions before comparing.
- `(kernel-unify a b)` — Lisp-facing primitive.
- `?N` syntax in sexp_to_kterm for metavariable references.

### Kernel Bool + Unit types
- `Bool`, `true`, `false` — boolean type with constructors.
- `(if b t f)` — Bool eliminator with computation rules:
  `if true t f → t`, `if false t f → f`.
- `Unit`, `*` — unit type with unique inhabitant.

### Tactic: assumption
- `(tactic-assumption)` — search context for hypothesis matching goal.
