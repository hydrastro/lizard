; 189-partitions.lisp -- the integer partition function p(n).
;
; p(n) counts the unordered ways to write n as a sum of positive integers.  Euler's
; pentagonal number theorem gives the recurrence p(n) = sum_{k>=1} (-1)^{k-1}
; ( p(n - k(3k-1)/2) + p(n - k(3k+1)/2) ).  The values are certified independently
; against the generating function sum p(n) x^n = prod 1/(1 - x^m): the two computations
; must agree on p(0)..p(N).  `must` raises on failure.

(import "cas/partitions.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'partitions-check-failed)))

(display "The integer partition function") (newline) (newline)

(display "1. small values") (newline)
(display "    p(0..12) = ") (display (partition-list 12)) (newline)
(must "p(0..10) = (1 1 2 3 5 7 11 15 22 30 42)" (equal? (partition-list 10) (list 1 1 2 3 5 7 11 15 22 30 42)))
(must "p(5) = 7"    (= (partition 5) 7))
(must "p(10) = 42"  (= (partition 10) 42))
(newline)

(display "2. larger values (exact)") (newline)
(display "    p(50)  = ") (display (partition 50)) (newline)
(display "    p(100) = ") (display (partition 100)) (newline)
(must "p(50) = 204226"        (= (partition 50) 204226))
(must "p(100) = 190569292"    (= (partition 100) 190569292))
(newline)

(display "3. cross-check against the generating function 1 / prod (1 - x^m)") (newline)
(must "agree on p(0..20)"  (partitions-ok? 20))
(must "agree on p(0..40)"  (partitions-ok? 40))
(must "agree on p(0..60)"  (partitions-ok? 60))
(newline)

(display "all partition checks passed.") (newline)
