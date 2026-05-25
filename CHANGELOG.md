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
