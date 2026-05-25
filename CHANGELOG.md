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
