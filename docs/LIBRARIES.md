# lizard Standard Libraries

The `lib/` directory contains importable Lisp libraries. Load them with
`(import "name.lisp")`. The `lib/` directory is on the default search path.

## lib/match.lisp — Functional toolkit (65+ functions)

**Higher-order:** compose, pipe, ->, ->>, curry, uncurry, flip, complement,
juxt, memoize, identity, const

**Lists:** range, enumerate, zip, flatten, take, drop, any, every, partition,
sort, repeat, interleave, scan, iterate, unfold, tabulate

**Aggregation:** sum, product, minimum, maximum, reduce, frequencies, group-by

**Alists:** alist-ref, alist-set, assoc-update

**Strings:** string-repeat

## lib/data.lisp — Persistent data structures

**Stack (LIFO):** stack-empty, stack-empty?, stack-push, stack-peek,
stack-pop, stack-size

**Queue (FIFO):** queue-empty, queue-empty?, queue-enqueue, queue-peek,
queue-dequeue — banker's queue with amortized O(1) operations

**Set:** set-empty, set-member?, set-add, set-remove, set-from-list,
set-union, set-intersect, set-difference, set-size

**Binary Search Tree:** bst-empty, bst-insert, bst-lookup, bst-inorder,
bst-from-list — ordered map with sorted traversal

**Priority Queue:** pqueue-empty, pqueue-insert, pqueue-min, pqueue-remove-min

## lib/church.lisp — Church encodings

Pure lambda-calculus encodings of data:

**Booleans:** church-true, church-false, church-if, church-and, church-or,
church-not, church->bool

**Numerals:** church-zero, church-succ, church-add, church-mul, church-exp,
church->int, int->church

**Pairs:** church-pair, church-fst, church-snd

**Lists:** church-nil, church-cons, church-list-sum

## lib/algorithms.lisp — Classic algorithms

**Sorting:** quicksort, mergesort, merge

**Searching:** linear-search, binary-search

**Number theory:** factorial, fib, my-gcd, prime?, primes-up-to,
collatz-length

**Strings:** palindrome?, char-count

## Example usage

```lisp
(import "match.lisp")
(import "data.lisp")
(import "algorithms.lisp")

(quicksort '(5 2 8 1 9))          ; => (1 2 5 8 9)
(set-union (set-from-list '(1 2 3))
           (set-from-list '(3 4 5))) ; => (5 4 1 2 3)
(bst-lookup (bst-from-list '((5 "a") (3 "b"))) 3) ; => "b"
(primes-up-to 20)                 ; => (2 3 5 7 11 13 17 19)
```

## Examples demonstrating libraries

- `examples/84-data-structures.lisp` — stacks, queues, sets, BSTs
- `examples/85-church-encodings.lisp` — lambda calculus
- `examples/86-algorithms.lisp` — sorting, searching, number theory

## lib/test.lisp — Unit testing framework

**Setup:** test-begin, test-end

**Assertions:** check-equal?, check-true, check-false, check-not-equal?,
check-all

```lisp
(import "test.lisp")
(test-begin "my suite")
(check-equal? (+ 1 2) 3 "addition")
(check-true (even? 4) "four is even")
(test-end)
```

## lib/matrix.lisp — Linear algebra

**Construction:** matrix-build, matrix-identity, matrix-zero

**Accessors:** matrix-rows, matrix-cols, matrix-ref

**Operations:** matrix-add, matrix-sub, matrix-scale, matrix-transpose,
matrix-mul, matrix-trace, dot-product

**Vectors:** vec-add, vec-sub, vec-scale, vec-norm-sq

**Helper:** map2 (element-wise binary map — lizard's map takes one list)

## lib/streams.lisp — Lazy infinite streams

**Core:** stream-cons-fn, stream-head, stream-tail, stream-empty?,
stream-take, stream-map, stream-filter, stream-iterate, stream-sum

**Generators:** naturals-from, nats, fib-stream, primes-stream (lazy sieve)

```lisp
(import "streams.lisp")
(stream-take 10 nats)              ; (0 1 2 3 4 5 6 7 8 9)
(stream-take 15 (primes-stream))   ; lazy Sieve of Eratosthenes
(stream-take 15 (fib-stream))      ; Fibonacci stream
```

## New examples
- `examples/87-testing-matrix.lisp` — test framework + linear algebra
- `examples/88-lazy-streams.lisp` — infinite sequences

## lib/concurrent.lisp — Concurrency patterns (Track C)

Higher-level coordination built on atoms:

**Counter:** make-counter, counter-inc!, counter-dec!, counter-add!,
counter-get, counter-reset!

**Accumulator:** make-accumulator, accumulate!, accumulator-values,
accumulator-count

**Cell (with history):** make-cell, cell-set!, cell-get, cell-history,
cell-rollback!

**Lazy reference:** make-lazy-ref, lazy-ref-get (compute once, cache)

**Registry:** make-registry, registry-put!, registry-get, registry-keys,
registry-size

**Semaphore:** make-semaphore, semaphore-acquire!, semaphore-release!,
semaphore-available

## lib/proof.lisp — Proof helpers (Track K)

**Type checking:** has-type?, defeq?, normalize

**Verified combinators:** install-combinators! (idNat, constZero, mySucc, idBool)

**Proof scripts:** prove-refl, prove-exact, prove-left, prove-right

**Propositions-as-types:** implies-self, nat-and-bool, nat-or-bool,
proof-implies-self, proof-and (Curry-Howard correspondence)

**Reporting:** report-type, report-reduce

## lib/syntax-tools.lisp — Syntax helpers (Track R)

**Construction:** syntax-quote, syntax-unwrap, syntax-list?, syntax-symbol?

**Templates:** template-lookup, template-expand

**Form builders:** make-let, make-lambda, make-if, swap-args

**AST walking:** walk-form, count-nodes, collect-symbols

## New examples (Track advancement)
- `examples/89-concurrency.lisp` — Track C: atomic coordination
- `examples/90-propositions-as-types.lisp` — Track K: Curry-Howard
- `examples/91-syntax-tools.lisp` — Track R: syntax transformation

## lib/tactics-ext.lisp — Tactic combinators (Track K)

**Combinators:** tseq (sequence), tfirst (alternatives), ttry (swallow
failure), trepeat (repeat until failure)

**Tactic thunks:** t-intro, t-exact, t-refl, t-assumption, t-simpl,
t-split, t-left, t-right

**Strategies:** t-auto (refl|assumption), t-crush (simpl+auto),
prove-with, prove-by-computation, prove-sum

## lib/calc.lisp — Expression evaluator (self-hosting demo)

A complete arithmetic interpreter written in lizard:

**Environment:** calc-env-empty, calc-env-lookup, calc-env-extend

**Evaluator:** calc-eval, calc-apply, calc, calc-run

Supports +, -, *, /, mod, let-bindings, variables, and nesting.

```lisp
(calc '(let x 3 (let y 4 (+ (* x x) (* y y)))))  ; => 25
```

## New examples (roadmap milestones)
- `examples/93-calculator.lisp` — self-hosting interpreter
- `examples/94-tactic-combinators.lisp` — composable proof strategies

See `docs/ROADMAP.md` for the full phased plan.

## lib/pattern.lisp — Structural pattern matching (Evolution Plan Phase 4)

A genuine pattern matcher (the Phase 4 `match` exit criterion).

**Patterns:** `_` (wildcard), `x` (variable), literals, `(lit s)`,
`(cons pa pd)`, `(list p ...)`, `()` (empty list)

**Core:** match-pattern, match-run, binding-ref, no-match?

```lisp
(match-run val
  (list
    (list '() (lambda (b) "empty"))
    (list '(cons h t) (lambda (b) (binding-ref 'h b)))
    (list '_ (lambda (b) "other"))))
```

## lib/control.lisp — Control-flow forms (Evolution Plan Phase 4)

**Dispatch:** case-of, cond-of

**Conditionals:** when-do, unless-do (thunk-based, short-circuiting)

**Loops:** dotimes, while-do

**Threading:** thread-through, apply-when

## New examples (Evolution Plan Phase 4 milestones)
- `examples/95-pattern-matching.lisp` — structural match
- `examples/96-control-flow.lisp` — case, when, cond, loops

See `docs/LIZARD_EVOLUTION_PLAN.md` (the production roadmap).

## lib/format.lisp — String formatting & tables

**Strings:** ->string, repeat-str, pad-left, pad-right, pad-center

**Templates:** format-template ("Hello {}" + args)

**Tables:** render-table, column-widths

**Numbers:** format-int

## lib/json.lisp — JSON serialization

**Serialize:** to-json, to-json-pretty

**Construct:** json-object, json-field

```lisp
(to-json (list 1 2 3))                    ; [1, 2, 3]
(to-json (json-object (json-field 'name "Bob")))  ; {"name": "Bob"}
(to-json-pretty doc)                       ; indented multi-line
```

## lib/record.lisp — Record/struct system

**Construct:** make-record, record (variadic)

**Access:** record-get, record-has?, record-keys, record-values, record-fields

**Update (functional):** record-set, record-update, record-remove, record-merge,
record-map-values

**Types:** record-type (constructor + accessor generation)

```lisp
(define p (record 'name "Alice" 'age 30))
(record-get p 'name)                       ; "Alice"
(record-update p 'age (lambda (a) (+ a 1))) ; new record, age 31
```

## New example
- `examples/98-format-json-records.lisp` — formatting, JSON, records combined

## lib/parser.lisp — Parser combinators (Evolution Plan Phase 3)

A parser is `input-list → result`. Results are `(ok value rest)` or `(fail)`.

**Primitives:** p-any, p-lit, p-sat, p-return, p-fail-always

**Combinators:** p-map, p-seq, p-left, p-right, p-or, p-then2,
p-many, p-many1

**Helpers:** parse, parse-all, p-digit, p-number

```lisp
(parse-all (p-many (p-lit 'a)) '(a a a))    ; (a a a)
(p-value (parse (p-number) '(1 2 3 x)))      ; (1 2 3)
;; grammar: open x* close
(p-left (p-right (p-lit 'open) (p-many (p-lit 'x))) (p-lit 'close))
```

## lib/graph.lisp — Graph algorithms

Graphs are adjacency lists: `((a b c) (b d) ...)`.

**Structure:** graph-nodes, graph-neighbors, graph-has-edge?,
graph-edge-count, graph-add-edge

**Traversal:** graph-dfs, graph-bfs

**Analysis:** graph-reachable?, graph-find-path, graph-topo-sort,
graph-component, graph-out-degree, graph-in-degree

```lisp
(graph-dfs g 'a)            ; depth-first order
(graph-bfs g 'a)            ; breadth-first order
(graph-find-path g 'a 'e)   ; (a b d e)
(graph-topo-sort deps)      ; dependency build order
```

## New examples
- `examples/99-parser-combinators.lisp` — parsing (Phase 3)
- `examples/100-graph-algorithms.lisp` — DFS, BFS, topo-sort

## lib/metacircular.lisp — A Lisp interpreter in lizard

The classic metacircular evaluator: a complete Lisp dialect
implemented in lizard. Demonstrates computational self-reference.

**Environment:** menv-new, menv-lookup, menv-define

**Closures:** make-closure, closure?, closure-params/body/env

**Evaluator:** meta-eval, meta-apply, meta-run, meta-show

Supports: literals, symbols, quote, if, lambda, let, begin,
application, and 16 primitives (+, -, *, /, comparisons, cons,
car, cdr, list, null?, not).

```lisp
(meta-run '((lambda (x) (* x x)) 7))             ; 49
(meta-run '(let ((make-adder (lambda (n) (lambda (x) (+ x n)))))
             ((make-adder 5) 100)))               ; 105 (closures!)
```

## lib/rational.lisp — Exact rational arithmetic

Fractions in lowest terms with exact arithmetic (no float error).

**Construct:** make-rat (auto-normalizes), ->rat

**Arithmetic:** rat+, rat-, rat*, rat/, rat-negate, rat-reciprocal

**Compare:** rat=, rat<

**Aggregate:** rat-sum, rat-product, harmonic (exact H(n))

```lisp
(rat->string (rat+ (make-rat 1 2) (make-rat 1 3)))  ; "5/6"
(rat->string (harmonic 5))                           ; "137/60" (exact!)
```

## New examples (EXTREME drop)
- `examples/101-metacircular.lisp` — Lisp-in-Lisp
- `examples/102-rationals.lisp` — exact fractions

## lib/numtheory.lisp — Number theory (bignum showcase)

**Modular:** mod, gcd2, lcm2, ext-gcd, mod-expt (fast), mod-inverse

**Primes:** is-prime?, factorize, totient

**Big integers:** factorial, power, fib-fast (fast doubling)

```lisp
(mod-expt 2 1000 1000000007)   ; fast modular exponentiation
(factorial 30)                  ; 265252859812191058636308480000000
(fib-fast 200)                  ; 280571172992510140037611932413038677189525
```

## lib/combinatorics.lisp — Permutations & combinations

**Counts:** comb-factorial, choose, perm-count

**Enumeration:** permutations, combinations, power-set, cartesian,
partitions, derangements

```lisp
(permutations '(a b c))        ; all 6 orderings
(combinations '(a b c d) 2)    ; all 2-subsets
(power-set '(x y z))           ; all 8 subsets
(partitions 5)                 ; integer partitions
```

## New examples
- `examples/103-number-theory.lisp` — modular arithmetic, big integers
- `examples/104-combinatorics.lisp` — permutations, partitions

## lib/namespace.lisp — Module/namespace system (Phase 5 / Track R)

Value-level modules mirroring Racket's import forms.

**Construct:** make-module, exports, module?

**Access:** module-ref, module-provides?, module-exports

**Import forms:** import-all, import-only, import-except, import-rename,
import-prefix

**Namespaces:** make-namespace, namespace-add, namespace-ref,
import-conflicts

```lisp
(define m (make-module 'math (exports 'pi 314 'e 271)))
(module-ref m 'pi)                       ; 314
(import-only m '(pi))                     ; ((pi . 314))
(import-rename m '((pi . PI)))            ; ((PI . 314))
(import-prefix m 'math:)                  ; ((math:pi . 314) ...)
```

## lib/trace.lisp — Tracing & debugging (Phase 8)

**Tracing:** trace-fn1, trace-fn2 (logs call trees with indentation)

**Counting:** make-call-counter, counted

**Step measurement:** reset-steps!, tick-step!, get-steps, measure-steps

**Debug:** debug (inline pass-through), assert!, assert-equal!

```lisp
(define traced (trace-fn1 "fib" fib))    ; logs each call/return
(measure-steps "loop" (lambda () (work))) ; deterministic step count
(debug "x" (compute))                     ; print + pass through
```

## New examples
- `examples/105-namespaces.lisp` — modules, selective import, renaming
- `examples/106-tracing-debug.lisp` — call trees, counting, assertions

## lib/logic.lisp — Prolog-style logic engine (EXTREME)

Full logic programming: unification, a clause database, SLD
resolution with backtracking, multiple solutions.

**Terms:** variables (?X), atoms, compounds (functor arg...)

**Database:** db-fact, db-rule

**Core:** unify, walk, solve-goal, solve-goals

**Query:** query, query-var, provable?, solution-count, resolve

```lisp
(define db (list (db-fact '(parent tom bob))
                 (db-rule '(grandparent ?X ?Z)
                          '((parent ?X ?Y) (parent ?Y ?Z)))))
(query-var db '(grandparent tom ?gc) '?gc)   ; tom's grandchildren
(provable? db '(parent tom bob))             ; #t
```

## lib/regex.lisp — Regular expression engine (EXTREME)

Backtracking matcher with continuation-passing.

**Patterns:** lit, any, seq, alt, star, plus, opt, set

**Constructors:** rx-lit, rx-seq, rx-alt, rx-star, rx-plus, rx-opt,
rx-set, rx-string

**Match:** regex-match (full), regex-search (anywhere)

```lisp
(regex-match (rx-alt (rx-string "cat") (rx-string "dog")) "cat")  ; #t
(regex-match (rx-seq (rx-star (rx-set "ab")) (rx-lit "c")) "ababc") ; #t
```

## lib/lambda-calc.lisp — Lambda calculus (EXTREME)

Untyped lambda calculus with capture-avoiding substitution and
normal-order beta reduction.

**Terms:** variables, (lam x body), (app f a)

**Core:** free-vars, lc-subst, beta-step, lc-normalize, lc-steps

**Church:** church-term, church-decode, church-succ, church-plus,
church-mult

```lisp
(lc-normalize (list 'app '(lam x x) 'y) 100)        ; y
;; arithmetic by pure beta reduction:
(church-decode (lc-normalize
  (list 'app (list 'app church-plus (church-term 2)) (church-term 3)) 1000)) ; 5
```

## New examples (EXTREME drop)
- `examples/107-logic-engine.lisp` — Prolog (family tree, append relation)
- `examples/108-regex-engine.lisp` — pattern matching
- `examples/109-lambda-calculus.lisp` — Church arithmetic

## lib/syntax-rules.lisp — Macro engine with ellipsis (Track R headline)

Pattern-based macros with `...` (ellipsis) — the key Track R feature.

**Matching:** sr-match (patterns with vars, literals, _, ellipsis)

**Expansion:** sr-expand (templates with ellipsis iteration)

**Macros:** make-macro, macro-apply (multi-rule, first-match)

```lisp
(define m (make-macro '()
  (list (list '(my-list e ...) '(list e ...)))))
(macro-apply m '(my-list 1 2 3 4))     ; (list 1 2 3 4)

;; recursive-shape macro with multiple rules:
(make-macro '() (list
  (list '(my-and) '#t)
  (list '(my-and e) 'e)
  (list '(my-and e rest ...) '(if e (my-and rest ...) #f))))
```

## lib/cubical.lisp — Cubical type theory & HITs (Track Q)

Wraps the interval/path primitives into a HoTT toolkit.

**Interval:** i-var, i-meet (∧), i-join (∨), i-not (~), i-reduce

**Paths:** path, path-at, path-start, path-end, path-const,
path-invert, path-ap, connection-and, connection-or

**HITs:** s1-base/s1-loop/s1-rec (circle), interval-zero/one/seg,
susp-north/south/merid (suspension)

```lisp
(i-reduce (i-not (i-not (i-var 'i))))   ; ~~i = i  (involution)
(path-invert p)                          ; p⁻¹ = <i> p @ ~i
(s1-loop)                                ; loop : base = base
```

## New examples (research tracks)
- `examples/110-syntax-rules.lisp` — Track R: macros with ellipsis
- `examples/111-cubical-hits.lisp` — Track Q: interval algebra, HITs

## lib/csp.lisp — Cooperative concurrency (Track C)

Single-thread cooperative concurrency over atoms.

**Channels:** make-channel, chan-send!, chan-recv!, chan-peek,
chan-empty?, chan-size

**Scheduler:** make-scheduler, spawn!, run-scheduler!, repeat-task
(round-robin; tasks yield via (cons 'next thunk))

**Futures:** make-future, force-future (compute-once), future-map

**Agents:** make-agent, agent-send!, agent-send-all!, agent-state
(actor-style state + message handler)

```lisp
(define acct (make-agent 100 (lambda (bal m) (+ bal (car (cdr m))))))
(agent-send! acct '(deposit 50))      ; agent-state → 150
(force-future (make-future thunk))     ; compute-once
```

## lib/inductive.lisp — Inductive families (Track K)

Dependent types: Vec A n (length-indexed) and Fin n (bounded index).

**Vec:** vnil, vcons, vhead, vtail, vappend, vmap, vzip, list->vec,
vec->list, vec-len, vec-rec (eliminator)

**Fin:** fzero, fsuc, make-fin, fin-bound, fin-index

**Payoff:** vnth (total, safe indexing — Fin n indexes Vec A n)

```lisp
(vappend (list->vec '(1 2)) (list->vec '(3 4 5)))  ; Vec 2 ++ Vec 3 = Vec 5
(vnth vec5 (make-fin 5 2))   ; total: Fin 5 always valid for Vec _ 5
```

## New examples (research tracks)
- `examples/112-concurrency-csp.lisp` — Track C: channels, scheduler, agents
- `examples/113-inductive-families.lisp` — Track K: Vec, Fin, dependent types

## lib/rewrite.lisp — Term rewriting + induction (Track K)

**Matching:** rw-match (one-directional), rw-subst

**Rewriting:** rule, rewrite-root, rewrite-step, rewrite-normalize,
rewrite-trace

**Induction:** nat-induction, list-induction, verify-nat-property

**Equational reasoning:** eq-chain, eq-chain-result

```lisp
(rewrite-normalize arith-rules '(* (+ a 0) 1) 50)   ; => a
(verify-nat-property (lambda (n) (= (sum-to n) (gauss-formula n))) 20)  ; #t
```

## lib/macro-expand.lisp — Recursive hygienic expansion (Track R)

**Gensym:** gensym (fresh symbols)

**Env:** make-macro-env, macro-env-add, macro-lookup

**Expansion:** expand-all (to fixpoint), expand-once, expand-all-h

**Hygiene:** make-hygienic-macro, rename-ids (template binders → gensyms)

```lisp
(expand-all '(my-unless done (cleanup)) menv)  ; my-unless→my-when→if
(expand-all-h '(swap tmp y) hyg-menv)          ; template tmp renamed; no capture
```

## lib/hit-recursors.lisp — HIT eliminators (Track Q)

Recursors that satisfy the computation (β) rules.

**Circle:** s1-rec, s1-rec-check, loop-pt

**Suspension:** susp-rec, merid-pt

**Torus:** torus-rec, torus-loop-a/b

**Truncation:** trunc-rec, trunc-inj

**Demo:** winding-number (π₁(S¹) = ℤ)

```lisp
((s1-rec 'red (path-const 'red)) 'base)   ; β-rule: base ↦ red
```

## lib/stm.lisp — Software Transactional Memory (Track C)

Optimistic concurrency over versioned refs.

**Refs:** make-ref, ref-value, ref-version

**Transactions:** tx-begin, tx-read, tx-write!, tx-commit!

**Runner:** atomically (validate + retry), stm-transfer!, stm-update!

```lisp
(stm-transfer! account-a account-b 30)   ; atomic multi-ref update
(atomically (lambda (tx) (tx-write! tx r (+ (tx-read tx r) 1))))
```

## New examples (research tracks — extreme drop)
- `examples/114-rewrite-induction.lisp` — Track K
- `examples/115-hygienic-macros.lisp` — Track R
- `examples/116-hit-recursors.lisp` — Track Q
- `examples/117-stm.lisp` — Track C

## lib/hm-infer.lisp — Hindley-Milner type inference (Track K)

Algorithm W with unification and let-polymorphism.

**Types:** tvar, arrow, tpair; **Schemes:** mono, forall

**Core:** unify, apply-subst, instantiate, generalize, infer, type-of,
type->string

```lisp
(type->string (type-of '(lam x (lam y x))))            ; (t1 -> (t2 -> t1))
(type->string (type-of '(let id (lam x x) (app id id)))) ; (t6 -> t6)
```

## lib/syntax-case.lisp — Procedural macros (Track R)

Each clause's RHS is a procedure over the matched bindings.

**Clauses:** sc-clause (pattern fender transformer)

**Access:** sc-ref, sc-seq?; **Apply:** make-sc-macro, sc-apply, syntax-case

```lisp
(sc-clause '(square x) #t (lambda (b) (let ((x (sc-ref b 'x))) (list '* x x))))
(sc-clause '(classify n) (lambda (b) (number? (sc-ref b 'n))) ...)  ; fender
```

## lib/univalence.lisp — Glue, univalence, Kan comp (Track Q)

**Equivalences:** equiv-id, equiv-app-fwd/bwd, equiv-id-fwd

**Glue:** glue, glue-on-true/false, glue-make, glue-unglue

**Univalence:** univalence (ua), ua-id

**Kan:** kan-comp, kan-hcomp, kan-fill, comp-const, transp

```lisp
(ua-id 'A)               ; ua of identity equivalence
(glue-on-true 'A 'T 'e)  ; Glue on φ=1 → T
```

## lib/coroutine.lisp — CSP coroutines with blocking (Track C)

True blocking via a parking scheduler (no host continuations).

**Steps:** co-done, co-yield, co-send, co-recv

**Channels:** co-channel; **Scheduler:** make-corun, co-spawn!, co-run!

**Built-ins:** co-producer, co-consumer, co-transform

```lisp
(co-spawn! r (co-producer ch 5))
(co-spawn! r (co-consumer ch 5))   ; consumer blocks until producer sends
(co-run! r 1000)
```

## New examples (research tracks — extreme drop II)
- `examples/118-type-inference.lisp` — Track K: Algorithm W
- `examples/119-syntax-case.lisp` — Track R: procedural macros
- `examples/120-univalence.lisp` — Track Q: Glue + univalence
- `examples/121-coroutines.lisp` — Track C: blocking CSP

## lib/reader.lisp — Reader with #lang dialects (Track R → 100%)

A reader written in lizard: text → s-expressions, with dialects.

**Tokenize/parse:** tokenize, read-str, read-all, parse-tokens

**Dialects:** make-dialect, dialect-registry, read-program (honors
a leading (#lang NAME) directive)

**Samples:** identity-dialect, infix-dialect (a+b → (+ a b)),
quote-all-dialect

```lisp
(read-str "(f (g x) y)")                      ; (f (g x) y)
(read-program "(#lang infix) (1 + 2)" reg)    ; ((+ 1 2))
```

## lib/hit-compute.lisp — HIT computation & canonicity (Track Q → 100%)

The computational payoff of HITs.

**Loops as ℤ:** loop-id, loop-compose, loop-invert, loop-law-*

**Universal cover:** cover-transport, encode-loop, decode-winding

**Recursor:** s1-rec-loop-power; **Degrees:** map-degree, compose-degrees

**Canonicity:** check-canonicity, canonical-bool?, canonical-nat?,
s1-canonical-form

```lisp
(loop-compose 2 (loop-invert 2))    ; 0 — loop²·loop⁻² = refl
(cover-transport 3 0)               ; 3 — transport adds winding
```

## examples/124-capstone-pipeline.lisp — INTEGRATION

read → expand → type-infer → evaluate, chaining the reader,
macro engine, HM inference, and the self-hosting evaluator into a
single typed-language frontend.

## New examples (research tracks — completion)
- `examples/122-reader-lang.lisp` — Track R: reader + #lang
- `examples/123-hit-computation.lisp` — Track Q: winding numbers, canonicity
- `examples/124-capstone-pipeline.lisp` — all subsystems composed

## lib/cas.lisp — Symbolic computer algebra

Symbolic expressions, simplification, differentiation, basic integration.

**Build/predicates:** cas-const?, cas-var?, cas-op?, cas-sum, cas-prod, …

**Simplify:** simplify (algebraic identities)

**Calculus:** diff, derivative (diff+simplify), integrate

**Print:** cas->string

```lisp
(cas->string (derivative '(^ x 2) 'x))   ; "(2 * x)"
(cas->string (integrate '(^ x 2) 'x))    ; "(x^3 / 3)"
```

## lib/cas-proof.lisp — Proof-producing CAS (CAS → ZFC)

Attaches justifications to CAS steps and traces them to ZFC axioms.

**Database:** foundation-db (ZFC → reals → fields → limits → calculus),
rule-statement, rule-deps, axiom?

**Unfolding:** unfold-to-axioms, dep-tree

**Derivations:** diff-proof, deriv, print-derivation

**Foundations:** print-foundations, print-dep-layers

```lisp
(print-derivation (diff-proof '(^ x 2) 'x))   ; tree with rule citations
(unfold-to-axioms 'calc-power)                ; ZFC axioms it rests on
(print-foundations 'calc-product)             ; product rule → ZFC
```

See `docs/CAS.md` for the verified-CAS architecture and roadmap.

## New examples (CAS)
- `examples/125-cas-symbolic.lisp` — differentiation & integration
- `examples/126-cas-proof-to-zfc.lisp` — unfolding a derivative to ZFC
