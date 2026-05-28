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
