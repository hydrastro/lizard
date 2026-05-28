# Lizard — Usage Guide

A getting-started guide and a set of recipes for the standard library.

## Running code

```
./build/lizard               # interactive REPL
./build/lizard prog.lisp     # run a script
./build/lizard < prog.lisp   # pipe via stdin
```

## Importing libraries

Libraries live in `lib/` and are on the default search path:

```lisp
(import "match.lisp")        ; the functional toolkit (load this first; many libs use it)
(import "data.lisp")         ; data structures
(import "cas.lisp")          ; the computer algebra core
```

`match.lisp` defines helpers (`range`, `take`, `drop`, `zip`, `fold`s,
`compose`, …) that several other libraries build on, so import it first.

## Recipes

### Data structures
```lisp
(import "data.lisp")
(define s (set-from-list '(1 2 3)))
(set-union s (set-from-list '(3 4 5)))        ; {1,2,3,4,5}
(bst-lookup (bst-from-list '((5 "a") (3 "b"))) 3)  ; "b"
```

### Algorithms
```lisp
(import "match.lisp") (import "algorithms.lisp")
(quicksort '(5 2 8 1 9))      ; (1 2 5 8 9)
(primes-up-to 30)             ; (2 3 5 7 11 13 17 19 23 29)
```

### Lazy streams
```lisp
(import "streams.lisp")
(stream-take 10 (primes-stream))   ; first 10 primes, computed on demand
(stream-take 15 (fib-stream))
```

### Exact arithmetic
```lisp
(import "rational.lisp")
(rat->string (harmonic 5))         ; "137/60"  (exact)
(import "numtheory.lisp")
(mod-expt 2 1000 1000000007)       ; fast modular exponentiation
```

### Pattern matching
```lisp
(import "pattern.lisp")
(match-run val
  (list (list '()         (lambda (b) "empty"))
        (list '(cons h t) (lambda (b) (binding-ref 'h b)))
        (list '_          (lambda (b) "other"))))
```

### Macros (Track R)
```lisp
(import "syntax-rules.lisp")
(define m (make-macro '() (list (list '(my-list e ...) '(list e ...)))))
(macro-apply m '(my-list 1 2 3))   ; (list 1 2 3)
```

### Proofs & types (Track K)
```lisp
(import "hm-infer.lisp")
(type->string (type-of '(lam x x)))             ; "(t1 -> t1)"
(import "inductive.lisp")
(vnth (list->vec '(a b c)) (make-fin 3 1))      ; 'b
(import "rewrite.lisp")
(rewrite-normalize arith-rules '(* (+ a 0) 1) 50)  ; a
```

### Concurrency (Track C)
```lisp
(import "stm.lisp")
(define acct (make-ref 100))
(stm-update! acct (lambda (b) (+ b 50)))        ; atomic
(import "coroutine.lisp")
; producer/consumer with blocking channels — see examples/121
```

### Cubical / HoTT (Track Q)
```lisp
(import "cubical.lisp")
(i-reduce (i-not (i-not (i-var 'i))))           ; ~~i = i
(import "hit-compute.lisp")
(loop-compose 2 (loop-invert 2))                ; 0  (π₁(S¹)=ℤ)
```

### Computer algebra (CAS)
```lisp
(import "cas.lisp")
(cas->string (derivative '(^ x 2) 'x))          ; "(2 * x)"
(cas->string (integrate '(^ x 2) 'x))           ; "(x^3 / 3)"
(import "cas-proof.lisp")
(print-derivation (diff-proof '(^ x 2) 'x))     ; derivation w/ rule citations
(print-foundations 'calc-power)                 ; unfold down to ZFC axioms
```

## The capstone

`examples/124-capstone-pipeline.lisp` chains four subsystems:
read (Track R) → macro-expand (Track R) → type-infer (Track K) →
evaluate (self-hosting). Run it to see the tracks compose.

## Self-hosting demos

- `metacircular.lisp` — a Lisp interpreter in lizard
- `logic.lisp` — a Prolog-style engine (unification + backtracking)
- `regex.lisp` — a backtracking regex engine
- `lambda-calc.lisp` — beta reduction + Church arithmetic

## Tips

- `match.lisp`'s `map` analog handles one list; for element-wise binary
  ops use `map2` (defined in `matrix.lisp`) or a manual recursion.
- Numbers are arbitrary-precision (GMP). `123^12312` is fine.
- For exact fractions use `rational.lisp`; CAS works symbolically.
