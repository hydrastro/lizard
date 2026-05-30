; -*- lisp -*-
; ============================================================
;  BENCHMARKS — exercise the engine and the evaluator
; ============================================================
;
; Run with:
;   time ./build/lizard < examples/22-benchmarks.lisp
;
; These are not micro-benchmarks; they're rough sanity checks
; that lizard can handle nontrivial workloads.

; ------------------------------------------------------------
; Evaluator: tail recursion at scale
; ------------------------------------------------------------
(define (count-down n)
  (if (= n 0) 'done (count-down (- n 1))))

(display "tail-loop 100000:    ")
(display (count-down 100000)) (newline)

; ------------------------------------------------------------
; Evaluator: deep recursion via closures
; ------------------------------------------------------------
(define (make-counter)
  (define n 0)
  (lambda () (set! n (+ n 1)) n))

(define c (make-counter))
(define (drain n)
  (if (= n 0) 'done (begin (c) (drain (- n 1)))))

(display "counter 10000:       ")
(drain 10000)
(display c)
(display " (the closure, value held: ")
(display (c)) (display ")") (newline)

; ------------------------------------------------------------
; Numbers: bignum work
; ------------------------------------------------------------
(display "fact 500 length:     ")
(define (fact n) (if (= n 0) 1 (* n (fact (- n 1)))))
(display (string-length (number->string (fact 500))))
(newline)

; ------------------------------------------------------------
; Engine: identity reductions at scale
; ------------------------------------------------------------
(define (sym-chain n acc)
  (if (= n 0) acc (sym-chain (- n 1) (Id-sym acc))))

(display "reduce 5000 syms:    ")
(display (reduce (sym-chain 5000 (refl 'x))))
(newline)

(define (left-trans n acc)
  (if (= n 0) acc (left-trans (- n 1) (Id-trans acc (refl 'a)))))

(display "reduce 500 l-trans:  ")
(display (reduce (left-trans 500 (refl 'a))))
(newline)

; ------------------------------------------------------------
; Engine: many small equal? calls
; ------------------------------------------------------------
(define (loop n proc)
  (if (= n 0) 'done (begin (proc) (loop (- n 1) proc))))

(define t1 (refl 'x))
(define t2 (Id-sym (Id-sym (refl 'x))))
(display "10000x equal?:       ")
(loop 10000 (lambda () (equal? t1 t2)))
(display "done") (newline)

; ------------------------------------------------------------
; Engine: deep nested with reductions inside Pi codomain
; ------------------------------------------------------------
(define complex
  (Pi 'x 'A
    (Pi 'y 'B
      (Id-trans (Id-sym (Id-sym (refl 'p)))
                (Id-trans (refl 'p) (Id-sym (refl 'p)))))))

(display "nested Pi reduce:    ")
(display (reduce complex)) (newline)

; trans-inverse should fire here: trans(refl_p, sym(refl_p))
; reduces to refl. Then Id-trans of (refl p) and (refl p) reduces.
; The whole codomain ends up as refl. Let's check:
(display "is codomain refl?    ")
(display (equal? complex (Pi 'x 'A (Pi 'y 'B (refl 'p)))))
(newline)
