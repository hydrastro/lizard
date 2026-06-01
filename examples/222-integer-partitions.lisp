; 222-integer-partitions.lisp -- the partition function p(n) via the pentagonal number theorem.
;
; p(n) is the number of ways to write n as a sum of positive integers, order disregarded.
; Euler's pentagonal number theorem gives a fast alternating recurrence over the generalized
; pentagonal numbers, building the whole table p(0..n) in exact integer arithmetic.  The result
; is cross-checked against a completely independent counting recurrence (partitions of n into
; parts <= k) and against the classical value p(100) = 190569292.  `must` raises on failure.

(import "cas/partition.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'pp-check-failed)))

(display "the integer partition function") (newline) (newline)

(display "1. small values") (newline)
(display "    p(0..10) = ") (display (map partition (list 0 1 2 3 4 5 6 7 8 9 10))) (newline)
(must "p(4) = 5"   (= (partition 4) 5))
(must "p(5) = 7"   (= (partition 5) 7))
(must "p(10) = 42" (= (partition 10) 42))
(newline)

(display "2. the pentagonal recurrence agrees with direct counting") (newline)
(display "    (partitions of n into parts <= k, an independent method)") (newline)
(must "p(n) matches the counting recurrence for all n in 0..18" (part-range-ok? 18))
(newline)

(display "3. larger values, exact integer arithmetic") (newline)
(display "    ") (display (part-info 50)) (newline)
(display "    ") (display (part-info 100)) (newline)
(must "p(100) = 190569292 (the classical value)" (= (partition 100) 190569292))
(newline)

(display "all partition-function checks passed.") (newline)
