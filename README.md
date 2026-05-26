# Lizard

A Scheme interpreter written in C89, plus a notation playground for
exploring foundational type-theory designs in Lisp surface syntax.

```
___                     .-*''*-.
 '.* *'.        .|     *       _*
  _)*_*_\__   \.`',/  * EVAL .'  *
 / _______ \  = ,. =  *.___.'    *
 \_)|@_@|(_/   // \   '.   APPLY.'
   ))\_/((    //        *._  _.*
  (((\V/)))  //            ||
 //-\\^//-\\--            /__\
```

## What lizard is and isn't

**Lizard is a Scheme interpreter.** It evaluates s-expressions, has
the usual core forms (`define`, `lambda`, `let`, `cond`, `if`,
`begin`, `set!`, `quote`, `quasiquote`), arithmetic on arbitrary-
precision integers, tail-call optimization, lexical closures, a real
macro system with both `define-syntax`-style transformer lambdas and
`syntax-rules` (with basic hygiene), exception handling, vectors,
hash tables, and a bunch of other practical features described below.

**Lizard also has a type-theory notation playground.** You can write
`(Pi (x A) B)`, `(Sigma (n Nat) (Vec n))`, `(U 0)`, `(Uco -2)`,
`(Id A a b)`, `(refl x)`, `(judgment Γ t T)`, and so on. These are
**opaque carriers** — lizard does not check that types are well-
formed, that variables are bound in the right contexts, that a
judgment is derivable, or anything else of substance. They exist so
that the surface syntax of a proposed foundational type theory can be
sketched, walked, and pattern-matched on. They are notation, not
verification. Building a real type checker for any of this is a
separate project; this codebase does not contain one.

**One exception: the computational identity engine.** Lizard does
implement a real judgmental equality decision procedure for the
identity-type fragment (`refl`, `Id-sym`, `Id-trans`, `transport`,
their interactions). For *this fragment*, `(equal? a b)` decides
whether two terms are judgmentally equal by running rewrite rules to
normal form. Each rule is gated by a flag, so the rule set is
pluggable. This is the smallest piece of a HoTT-style type theory
that contains real computational content. See the "Computational
identity" section below.

## Layout

```
.
├── include/
│   └── lizard.h              Public API header.
├── src/
│   ├── lizard.c              Evaluator: eval, apply, expand_macros,
│   │                         expand_quasiquote, syntax-rules matcher.
│   ├── parser.c              S-expression parser + non-specializing
│   │                         datum reader (used inside quote, syntax-rules).
│   ├── tokenizer.c           Source → token stream, with string escapes.
│   ├── primitives.c          All built-in procedures.
│   ├── env.c                 Environment frames.
│   ├── mem.c                 Bump-arena heap, GMP allocator integration.
│   ├── printer.c             Scheme-style value printer.
│   ├── repl.c                Interactive REPL + script driver.
│   └── errors.h / en.h       Error code list + English messages.
├── tests/
│   ├── test_harness.h        TEST_ASSERT / TEST_ASSERT_EQ / TEST_ASSERT_STR.
│   ├── test_helpers.{h,c}    Façade for C unit tests.
│   ├── *_test.c              23 C-level tests linked against liblizard.a.
│   └── *.lisp + *.expected   4 golden-output end-to-end tests.
├── examples/
│   ├── 01-basics.lisp … 19-type-theory.lisp
│   └── prelude.lisp          User-level stdlib (map, filter, fold, range, …).
├── Makefile                  Top-level build.
└── flake.nix                 Nix flake (devShell + package).
```

## Build

### With Nix (recommended)

```sh
nix develop
make            # build/liblizard.{a,so} and build/lizard
```

`nix develop` puts `ds` and `gmp` on the search paths, so no extra
flags are needed.

To upgrade the pinned `ds` revision:

```sh
nix flake update
make clean && make
```

### Without Nix

```sh
make CPPFLAGS="-I/path/to/ds/include" LDFLAGS="-L/path/to/ds/lib"
```

### Other targets

```sh
make test       # build + run all C and golden-output tests
make examples   # run every examples/*.lisp through lizard
make install    # PREFIX=/usr/local by default
make clean
make distclean  # also remove gmon.out, vgcore.*, *~, etc.
```

## Run

```sh
./build/lizard                        # interactive REPL
./build/lizard < examples/06-scripting.lisp
echo '(load "examples/prelude.lisp") (display (sum (range 1 101))) (newline)' \
  | ./build/lizard
```

## Dependencies

- [`hydrastro/ds`](https://github.com/hydrastro/ds) — intrusive data structures.
- [`gmp`](https://gmplib.org) — arbitrary precision integers.

Lizard uses its own typedefs (`lz_list_t`, `lz_list_node_t`) anchored
on the underlying struct tags `linked_list` and `list_node`, so it
builds against **either** older ds (`list_t`) or newer ds
(`ds_list_t`) without source edits.

# Reference

## Core Scheme

Standard R5RS-flavoured forms, with a few intentional differences
documented as they come up.

### Definitions and assignment

```scheme
(define x 42)                        ; bind x to 42
(define (square n) (* n n))          ; shorthand for (define square (lambda (n) (* n n)))
(define (compose f g) (lambda (x) (f (g x))))
(set! x 100)                         ; mutation of an existing binding
```

`(define …)` and `(set! …)` return nil so script mode doesn't echo
`=> <procedure>` after every function definition.

### Conditionals and control

```scheme
(if test consequent alternative)
(cond ((p1) e1) ((p2) e2) (else e3))
(begin e1 e2 … en)                    ; sequence, last value returned
(and e1 e2 …)  (or e1 e2 …)           ; short-circuiting
(not x)
```

### Lambda — three parameter shapes

```scheme
(lambda (a b c) (+ a b c))            ; fixed arity
(lambda args (apply + args))          ; varargs — args is a list of all arguments
(lambda (a b . rest) (cons a (cons b rest)))   ; dotted-rest — fixed + variadic tail
```

### Tail-call optimization

Lizard reliably eliminates tail calls through `if`, `cond`, `begin`,
lambda body, and `let`. A 500,000-iteration tail-recursive loop runs
in O(1) C stack frames. Mutual recursion across both branches is
also TCO'd.

### Lexical closures

Variables captured by a lambda are accessed by reference; `set!` in
the enclosing scope is visible from inside the closure. Standard
counter / accumulator patterns work:

```scheme
(define (make-counter)
  (define n 0)
  (lambda () (set! n (+ n 1)) n))
```

## Numbers (bignums)

All integer arithmetic is arbitrary-precision via GMP. `+ - * / % <
> <= >= =` work as expected; in addition lizard exposes fast bignum
primitives: `arithmetic-shift`, `expt`, `gcd`, `lcm`, `quotient`,
`remainder`, `abs`, `square`, `modular-expt`. Each delegates directly
to GMP and runs at native speed.

## Strings

```scheme
(string-length s)
(string-append a b ...)               ; n-ary
(substring s start [end])             ; end defaults to length
(string=? a b)
(number->string n)                    ; bignum-aware
(string->number s)                    ; returns #f on parse failure
(symbol->string s)  (string->symbol s)
```

The tokenizer decodes `\n`, `\t`, `\r`, `\\`, `\"`, `\0` inside
string literals.

`write` distinguishes from `display` on strings: `(write "hi\n")`
prints `"hi\n"` (literal escape), `(display "hi\n")` prints `hi` then
a real newline. Other types are formatted identically.

## Vectors

Fixed-size, mutable, O(1) indexed.

```scheme
(make-vector n [fill])
(vector v1 v2 v3 ...)                 ; literal
(vector? x)
(vector-length v)
(vector-ref v i)
(vector-set! v i x)
(vector->list v)
(list->vector xs)
```

Printer renders as `#(a b c)`.

## Hash tables

Open-addressed with linear probing, doubles at load > 0.75. FNV-1a
hashing for strings/symbols; mpz-aware for bignum keys.

```scheme
(make-hash-table)
(hash? x)
(hash-set! h k v)
(hash-ref h k [default])              ; default defaults to #f
(hash-has-key? h k)
(hash-size h)
(hash-keys h)
(hash-remove! h k)
```

Printer renders as `#hash((k . v) (k . v))`.

## Exception handling

```scheme
(raise value)                         ; produce an error carrying value
(try thunk handler)                   ; call thunk; if it errors, call handler with the error
(error-object? x)                     ; predicate
(error-value err)                     ; unwrap the payload (works for both
                                      ; user-raised errors and system errors)
```

Errors short-circuit at every eval boundary — primitive arg
forcing, `if`/`cond` predicates, `begin`/lambda-body intermediate
expressions — so `(display (raise 'oops))` does not silently print
`oops`; the error propagates up to the nearest `try` or to the REPL.

The two primitives `error-object?` and `error-value` are exempt from
auto-propagation, because they exist precisely to receive errors as
data.

```scheme
(define (safe-div a b)
  (try (lambda () (/ a b))
       (lambda (err)
         (display "caught: ") (display (error-value err)) (newline)
         0)))
```

## Macros

Two flavours:

### Transformer-lambda style (lizard-original)

```scheme
(define-syntax twice (lambda (x) `(* 2 ,x)))
(twice 21)                            ; => 42
```

The transformer is a regular lambda receiving its arguments as
unevaluated AST nodes. Quasiquote builds the expansion.

### syntax-rules with hygiene

```scheme
(define-syntax swap!
  (syntax-rules ()
    ((_ a b) (let ((tmp a)) (set! a b) (set! b tmp)))))

(define x 1) (define y 2)
(swap! x y)                           ; x=2, y=1

(define tmp 99) (define z 100)
(swap! tmp z)                         ; tmp=100, z=99 — no capture
```

Supported: literal symbol identifiers, pattern variables, `_`
wildcard, multi-clause dispatch by structure, basic hygiene
(gensym-rename of identifiers introduced in `let`/`lambda` binding
positions). Not yet supported: ellipsis (`...`).

### gensym

`(gensym [prefix])` returns a fresh unique symbol each call. Useful
for hand-rolled hygienic-ish macros.

## Reflection

```scheme
(type-of x)                           ; symbol naming the runtime type
(defined? 'name)                      ; #t/#f without throwing
(env-keys)                            ; list of every bound symbol in scope
(procedure-arity f)                   ; integer, or 'variadic for primitives
```

`type-of` covers: `number`, `string`, `symbol`, `boolean`, `nil`,
`pair`, `procedure`, `primitive`, `macro`, `vector`, `hash`, `error`,
plus the TT-notation types listed below.

# Type-theory notation playground

**Read the caveat at the top: none of these forms are checked.** They
are first-class lizard values you can construct, print, store in
lists, pattern-match on, and walk programmatically. There is no
verification that any of them are well-formed in any type theory.

## The four interaction-net primitives

```scheme
(Pi binder domain codomain)           ; (Pi (x A) B) — dependent function type
(Sigma binder domain codomain)        ; (Sigma (n A) B) — dependent pair
(@ fun arg)                           ; explicit type-level application
(Sum left right)                      ; coproduct A + B
```

Use `'()` (or any non-symbol) as the binder for non-dependent forms.
Each has matching predicates (`Pi?`, `Sigma?`, `app?`/`@?`, `Sum?`)
and accessors (`Pi-binder`, `Pi-domain`, `Pi-codomain`, etc.).

## Universes and couniverses

```scheme
(U n)                                 ; universe at integer level n
(Uco n)                               ; couniverse at integer level n
(U? x)  (Uco? x)
(U-level x)  (Uco-level x)
```

Universes are indexed from `-1` upward by convention: `(U -1)` for
primitive values, `(U 0)` for types of values, `(U n+1)` for types of
types living in `(U n)`.

Couniverses are indexed from `-2` upward by convention. The
[context layer](#context-layer-couniverse-stratified) below makes
this stratification concrete.

## Identity and reflexivity

```scheme
(Id type a b)                         ; identity type Id_A(a, b)
(refl x)                              ; reflexivity witness
```

With accessors `Id-domain`, `Id-a`, `Id-b`, `refl-value`.

## Inductive and coinductive declarations

```scheme
(Inductive 'Nat 'Z (list 'S (Pi '() 'Nat 'Nat)))
(Coinductive 'Stream (list 'head 'A) (list 'tail (@ 'Stream 'A)))
```

The constructor and destructor specs are stored opaquely. Accessors:
`Inductive-name`, `Inductive-constructors`, `Coinductive-name`,
`Coinductive-destructors`.

## Annotations

```scheme
(annot term type)                     ; (: term type) when printed
```

A way to write down a type claim about a term as a value. Not
checked. Accessors: `annot-term`, `annot-type`.

# Computational identity — a real engine

This is the part of lizard that does genuine work beyond notation.
It is a **judgmental equality engine** for the identity-type fragment.

```scheme
(reduce t)        ; Normalize t under the identity rewrite rules.
(equal? a b)      ; Decide judgmental equality: reduce both, compare.
```

The built-in rewrite rules:

```
sym(refl_a)              -->  refl_a
sym(sym(p))              -->  p
trans(refl_a, p)         -->  p
trans(p, refl_a)         -->  p
trans(refl_a, sym(refl_a)) -->  refl_a    (* via trans-refl-l + sym-refl *)
trans(trans(p,q), r)     -->  trans(p, trans(q,r))
transport(refl_a, x)     -->  x
(@ (Lambda 'x b) a)      -->  b[a/x]      (* pi-beta, capture-avoiding *)
```

Plus structural recursion — rules apply inside subterms (including
inside `Pi`, `Sigma`, `Id`, `App`, `Sum`, `Lambda` containers) before
being applied at the head. The system is terminating (a lexicographic
measure on (node-count, left-weight) strictly decreases on every
step) and confluent on this fragment (critical pairs close).

### Computational lambda calculus

The TT layer has its own lambda calculus, separate from Lisp's. The
constructor is `(Lambda 'x body)`. The reducer fires pi-beta with
capture-avoiding substitution:

```scheme
(reduce (@ (Lambda 'x 'x) 'a))         ; => a
(reduce (@ (Lambda 'x (refl 'x)) 'q))  ; => (refl q)

;; Two-stage beta
(reduce (@ (@ (Lambda 'f (Lambda 'x (@ 'f 'x)))
              (Lambda 'y 'y))
           'q))                        ; => q

;; Capture avoidance — the inner binder gets alpha-renamed when
;; substitution would otherwise capture
(reduce (@ (Lambda 'y (Lambda 'x 'y)) 'x))
;; => (Lambda x_N x), where x_N is fresh
```

### Alpha-equivalence

`equal?` compares terms modulo renaming of bound variables. Free
variables stay distinguished:

```scheme
(equal? (Pi 'x 'A 'x) (Pi 'y 'A 'y))   ; => #t  — both bind codomain
(equal? (Pi 'x 'A 'x) (Pi 'x 'A 'y))   ; => #f  — y is free
(equal? (Lambda 'x 'x) (Lambda 'y 'y)) ; => #t
(equal? (Pi 'x 'A (Pi 'y 'B 'x))
        (Pi 'a 'A (Pi 'b 'B 'a)))      ; => #t  — nested binders

### Engine architecture

The reducer uses **bottom-up normalization with memoization**:

1. Recursively normalize all subterms exactly once.
2. Apply head rewrites until none fire. Each rewrite may produce
   new subterms; recursively normalize those.
3. Cache `t → normal_form(t)` in a per-call hash table so that
   structurally-equal subterms sharing a pointer reduce once.

This is O(node-count × rule-depth) — effectively linear for typical
workloads. A workload that previously OOMed (10,000 nested syms,
100 equal? calls) now runs in 220ms. A 5,000-sym chain plus 500
left-associated trans plus 10,000 equal? calls plus a 100,000-step
tail loop runs the whole `examples/22-benchmarks.lisp` in 220ms.

### Limitations

The engine is **stack-bound**: `normalize_rec` is recursive in
subterm depth, so chains deeper than ~30,000 nodes overflow the C
stack. An iterative work-stack version would lift this; not done
yet. For practical use the limit is well above anything you'd
construct by hand.

**Now implemented:** alpha-equivalence (de-Bruijn-style binder
comparison) and pi-beta reduction (capture-avoiding substitution
with fresh-name generation when needed).

**Not yet implemented:** Sigma projections (`pi_1`, `pi_2` don't
reduce on pairs), J-rule reduction on refl, universe-level
arithmetic, congruence under arbitrary App.

## Pluggable rules — the flag system

Every reduction rule is gated by a flag. Flags are global, named by
symbol, boolean-valued:

```scheme
(flag-get 'reduce-sym-involutive)     ; #t (default)
(flag-set! 'reduce-sym-involutive #f) ; turn the rule off
(flag-list)                           ; list all registered flags
```

The flags currently defined:

| Flag | Effect |
|---|---|
| `reduce-sym-refl` | `sym(refl) -> refl` |
| `reduce-sym-involutive` | `sym(sym(p)) -> p` |
| `reduce-trans-refl-l` | `trans(refl, p) -> p` |
| `reduce-trans-refl-r` | `trans(p, refl) -> p` |
| `reduce-trans-inverse` | `trans(refl_a, sym(refl_a)) -> refl_a` (subsumed by trans-refl-l + sym-refl) |
| `reduce-trans-assoc` | `trans(trans(p,q),r) -> trans(p, trans(q,r))` |
| `reduce-transport-refl` | `transport(refl, x) -> x` |
| `reduce-pi-beta` | `(@ (Lambda 'x b) a) -> b[a/x]` |
| `reduce-structural` | apply rules inside subterms |

With a flag off, the corresponding rule does not fire, and `equal?`
will not equate terms that differ along that rule. This makes
different rule subsets behave like different theories — the
beginnings of a lambda-cube-style modularity.

## What this engine is NOT

I want to be explicit because the surface looks more impressive than
the contents are.

- **It is not a type checker.** Nothing verifies that the `p` in
  `(Id-sym p)` is actually an identity proof. The engine is conversion
  only — given two terms (well-typed or not), it decides whether they
  reduce to the same normal form.
- **It covers only the identity-type fragment.** Pi and Sigma do not
  reduce. Applications do not beta-reduce. Universes are compared
  structurally only. The fragment is the part that matters for HoTT's
  identity-type story, but the rest of HoTT isn't here.
- **It is not alpha-equivalent.** Terms with bound variables under
  different names compare unequal. A proper implementation would use
  de Bruijn levels; this one doesn't yet.
- **It does not check well-formedness, well-typing, or anything else
  about the input.** Garbage in, structurally-reduced garbage out.

## Extending the engine

Adding a new rewrite rule is a small, scoped task. The location is
`src/tt_equality.c`. The steps:

1. Add a flag in `lizard_tt_flags_init`.
2. Add a case in `lizard_tt_step` that checks the flag and applies
   the rule, returning the rewritten term and setting `*changed = 1`.
3. Verify termination — the lexicographic measure must still
   decrease.
4. Verify confluence — check critical pairs against existing rules.
5. Write a test in `tests/tt_equality_test.c`.

This is the **extension point** for adding more computational
content. The architecture is intentionally small so you can read all
of it.

# Context layer — couniverse-stratified

This is the part of the playground designed to make the
universe/couniverse stratification *queryable at runtime*. Four AST
node types, each with a hard-coded couniverse level:

| Form | Lives at | Built by |
|---|---|---|
| `(variable 'x type)` | `Uco -2` | `variable` |
| `(context v1 v2 ...)` | `Uco -1` | `context`, `empty-context`, `context-extend` |
| `(substitution from to mappings)` | `Uco 0` | `substitution` |
| `(judgment ctx term type)` | `Uco 0` | `judgment` |

The central operation:

```scheme
(uco-level x)                         ; -2, -1, or 0 for context-layer
                                      ; values; #f otherwise
```

This is the *only* place lizard hard-codes the stratification.
Everything else is opaque construction.

## Variables

```scheme
(variable 'x (U 0))
(variable-name v)                     ; => 'x
(variable-type v)                     ; => (U 0)
(variable? x)                         ; predicate
```

## Contexts

```scheme
(empty-context)                       ; () context
(context (variable 'x (U 0))
         (variable 'y (Pi '() 'Nat 'Nat)))

(context-extend ctx variable)         ; non-mutating; returns a new context
(context-lookup ctx 'name)            ; innermost-wins shadowing, #f on miss
(context-bindings ctx)                ; list of variables in declaration order
(context-length ctx)                  ; integer
```

Contexts are printed as `(context ...)`. They are at couniverse level
−1.

## Substitutions

```scheme
(substitution source-ctx target-ctx mappings)
(substitution? x)
(substitution-source s)
(substitution-target s)
```

`mappings` is whatever list of `(name . term)` pairs you want — not
validated against the source's domain. Substitutions are at
couniverse level 0.

## Judgments

```scheme
(judgment ctx term type)
(judgment? x)
(judgment-context j)
(judgment-term j)
(judgment-type j)
```

Judgments print as `(Γ |- term : type)` so they read like the
conclusion of an inference rule. They package three pieces of data;
they do not check anything.

# What is *not* in lizard

I want to be explicit because the surface looks like a proof
assistant and isn't one.

- **No type checking.** `(Pi (x Nat) banana)` constructs fine. So
  does `(judgment Γ t T)` for any unrelated `t` and `T`.
- **No conversion / definitional equality.** `(Id Nat (+ 2 2) 4)` and
  `(Id Nat 4 4)` are structurally distinct values.
- **No computational identity.** `refl` is just a tag.
- **No checked inductive definitions.** Strict positivity, coverage,
  termination — none of these are verified.
- **No proof obligations.** A `judgment` value is the *shape* of a
  conclusion. It is not a proof of anything.
- **No universe polymorphism, no cumulativity check, no Uco-level
  enforcement** beyond the runtime tag returned by `uco-level`.

This is a notation playground. To make any of these checks real
requires a separate kernel — a bidirectional type checker with
normalization-by-evaluation for conversion. That is a multi-month
project and is not what lizard is.

# Test suite

```
$ make test
23/23 C tests passing
  arith bignum closures control deep_recursion errors exceptions
  fastprims hashes higher_order lambda lists macros quasiquote
  reflection strings syntax_rules tco tt tt_context tt_equality
  tt_identity varargs vectors
4/4 Lisp golden tests passing
  bignum_smoke error_propagation qsort scripting
All tests passed.
```

# Examples

23 self-contained example files in `examples/`, all of which run
under lizard:

- **01–06** basics, recursion, lists, macros, control flow, scripting
- **07** bignum demonstrations
- **08–12** sort, sieve of Eratosthenes, Y combinator, Towers of
  Hanoi, N-queens
- **13** meta-circular evaluator skeleton
- **14** performance — a 1,000,000-iteration tail loop, `2^1,000,000`,
  toy RSA
- **15** reflection and ad-hoc records via tagged conses
- **16** runtime pattern matching, symbolic differentiation
- **17** string manipulation
- **18** the "extreme" example combining varargs, vectors, hash
  tables, exceptions, and gensym
- **19** the type-theory notation playground
- **20** identity manipulation notation (no engine)
- **21** computational identity — the engine actually deciding
  judgmental equality on the identity fragment
- **22** performance benchmarks — exercises both evaluator and engine
- **23** pi-beta reduction, alpha-equivalence, capture-avoidance,
  Church booleans — lambda calculus in the engine

# Contributing

- Format with `clang-format`.
- Run `make test` before submitting.
- Run under Valgrind or ASAN when touching memory paths:
  ```sh
  CPPFLAGS="… -fsanitize=address -g -O0" \
  LDFLAGS="… -fsanitize=address" make clean all
  ```
