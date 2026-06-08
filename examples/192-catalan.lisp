; 192-catalan.lisp -- Catalan numbers and binomial-coefficient identities.
;
; Binomials are built by the exact multiplicative rule (integer at every partial product),
; and the Catalan numbers are taken in closed form C_n = C(2n,n)/(n+1).  Each result is
; gated by a classical identity used as an independent check: the Catalan convolution and
; ratio recurrences, Pascal's rule, the row sum 2^n and alternating sum 0, Vandermonde's
; identity, and the hockey-stick identity.  `must` raises on failure.

(import "cas/catalan.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'catalan-check-failed)))
(define (all? l) (cond ((null? l) #t) ((car l) (all? (cdr l))) (else #f)))

(display "Catalan numbers and binomial identities") (newline) (newline)

(display "1. binomial coefficients (exact, no factorials)") (newline)
(display "    C(5,2) = ") (display (binom 5 2)) (display ", C(52,5) = ") (display (binom 52 5)) (display " (poker hands)") (newline)
(must "C(5,2) = 10"        (= (binom 5 2) 10))
(must "C(10,3) = 120"      (= (binom 10 3) 120))
(must "C(52,5) = 2598960"  (= (binom 52 5) 2598960))
(must "C(0,0) = 1"         (= (binom 0 0) 1))
(must "C(6,7) = 0"         (= (binom 6 7) 0))
(newline)

(display "2. Catalan numbers") (newline)
(display "    C_0..C_10 = ") (display (catalan-list 10)) (newline)
(must "C_0..C_10 are 1,1,2,5,14,42,132,429,1430,4862,16796"
      (equal? (catalan-list 10) (list 1 1 2 5 14 42 132 429 1430 4862 16796)))
(must "Catalan convolution holds for n = 0..15" (all? (map catalan-conv-ok? (range 0 15))))
(must "Catalan ratio recurrence holds for n = 0..15" (all? (map catalan-ratio-ok? (range 0 15))))
(newline)

(display "3. binomial identities") (newline)
(must "Pascal's rule at (10,4)"        (pascal-ok? 10 4))
(must "row sum = 2^n for n = 0..20"    (all? (map rowsum-ok? (range 0 20))))
(must "alternating sum = 0 for n = 1..20" (all? (map altsum-ok? (range 1 20))))
(must "Vandermonde (7,5,4)"            (vandermonde-ok? 7 5 4))
(must "Vandermonde (12,9,7)"           (vandermonde-ok? 12 9 7))
(must "hockey-stick (10,3)"            (hockey-ok? 10 3))
(must "hockey-stick (15,5)"            (hockey-ok? 15 5))
(newline)

(display "all Catalan / binomial checks passed.") (newline)
