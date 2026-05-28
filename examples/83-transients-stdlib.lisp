; -*- lisp -*-
; ============================================================
;  EXAMPLE 83 — Transients + expanded stdlib
; ============================================================

(display "=== TRANSIENTS (Track C.3) ===") (newline)
(newline)

; Build a large persistent vector efficiently
(display "  Building pvec of 100 elements via transient:") (newline)
(define t (transient! (pvec)))
(define (fill-loop! t n)
  (if (= n 100) t
    (fill-loop! (conj! t (* n n)) (+ n 1))))
(define big-vec (persistent! (fill-loop! t 0)))
(display "    pvec-count: ") (display (pvec-count big-vec)) (newline)
(display "    first 5: ")
(display (list (pvec-ref big-vec 0) (pvec-ref big-vec 1)
               (pvec-ref big-vec 2) (pvec-ref big-vec 3)
               (pvec-ref big-vec 4))) (newline)
(display "    element 10 (100): ") (display (pvec-ref big-vec 10)) (newline)
(newline)

; Transient round-trip preserves immutability
(define v1 (pvec 1 2 3))
(define t1 (transient! v1))
(conj! t1 4)
(conj! t1 5)
(define v2 (persistent! t1))
(display "  Original v1: ") (display v1) (newline)
(display "  After transient+conj: ") (display v2) (newline)
(newline)

; ═══════════════════════════════════════
(display "=== EXPANDED STDLIB ===") (newline)
(newline)
(import "match.lisp")

(display "  sum: ") (display (sum '(1 2 3 4 5))) (newline)
(display "  product: ") (display (product '(1 2 3 4 5))) (newline)
(display "  minimum: ") (display (minimum '(5 3 1 4 2))) (newline)
(display "  maximum: ") (display (maximum '(5 3 1 4 2))) (newline)
(newline)

(display "  scan +: ") (display (scan + 0 '(1 2 3 4 5))) (newline)
(display "  tabulate square 5: ") (display (tabulate (lambda (n) (* n n)) 5)) (newline)
(display "  iterate double 5x: ") (display (iterate (lambda (x) (* x 2)) 5 1)) (newline)
(newline)

(display "  interleave: ")
(display (interleave '(a b c) '(1 2 3))) (newline)
(newline)

(display "  frequencies: ")
(display (frequencies '(a b a c b a))) (newline)
(newline)

(display "  group-by even?:") (newline)
(display "    ") (display (group-by even? '(1 2 3 4 5 6))) (newline)
(newline)

; ═══════════════════════════════════════
(display "=== STRING OPS ===") (newline)
(newline)
(display "  string-reverse: ") (display (string-reverse "desserts")) (newline)
(display "  char->integer 'Z': ") (display (char->integer "Z")) (newline)
(display "  integer->char 97: ") (display (integer->char 97)) (newline)
(display "  string->list: ") (display (string->list "abc")) (newline)
(display "  list->string: ") (display (list->string '("x" "y" "z"))) (newline)
(newline)

; ═══════════════════════════════════════
(display "=== NUMBER PREDICATES ===") (newline)
(newline)
(display "  (zero? 0) = ") (display (zero? 0)) (newline)
(display "  (even? 42) = ") (display (even? 42)) (newline)
(display "  (odd? 7) = ") (display (odd? 7)) (newline)
(display "  (min 3 7) = ") (display (min 3 7)) (newline)
(display "  (max 3 7) = ") (display (max 3 7)) (newline)
(newline)

(display "=== End ===") (newline)
