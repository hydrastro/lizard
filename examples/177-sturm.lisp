; 177-sturm.lisp -- exact real-root counting and isolation via Sturm sequences.
;
; The canonical Sturm chain p_0=p, p_1=p', p_{k+1}=-rem(p_{k-1},p_k) lets us count the
; DISTINCT real roots in a half-open interval as the drop V(a) - V(b) in the number of
; sign variations of the chain.  Making p squarefree first means every root is simple,
; so the count is exact.  All real roots lie inside the strict Cauchy bound, and
; bisecting at non-root midpoints isolates each root in a rational interval that shows a
; strict sign change -- an independent certificate, on top of the rigorous Sturm count.
; `must` raises on failure.

(import "cas/sturm.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'sturm-check-failed)))

(display "Real-root counting and isolation -- Sturm sequences") (newline) (newline)

(display "1. root counts -- distinct real roots") (newline)
(must "x^2-2 has 2 real roots"                 (= (num-real-roots (list -2 0 1)) 2))
(must "x^2+1 has 0 real roots"                 (= (num-real-roots (list 1 0 1)) 0))
(must "product of x-1,x-2,x-3 has 3"           (= (num-real-roots (list -6 11 -6 1)) 3))
(must "x^3-2 has 1 real root"                  (= (num-real-roots (list -2 0 0 1)) 1))
(must "x^4-5x^2+4 has 4"                       (= (num-real-roots (list 4 0 -5 0 1)) 4))
(must "x-1 squared times x+2: 2 DISTINCT"      (= (num-real-roots (list 2 -3 0 1)) 2))
(must "x^5-x has 3"                            (= (num-real-roots (list 0 -1 0 0 0 1)) 3))
(newline)

(display "2. counting in a chosen half-open interval a < x <= b") (newline)
(must "x^2-2 has 1 root with 0 < x <= 2"       (= (count-real-roots (list -2 0 1) 0 2) 1))
(must "x^2-2 has 2 roots with -2 < x <= 2"     (= (count-real-roots (list -2 0 1) -2 2) 2))
(must "cubic has 2 roots with 1 < x <= 3"      (= (count-real-roots (list -6 11 -6 1) 1 3) 2))
(must "cubic has 0 roots with 3 < x <= 10"     (= (count-real-roots (list -6 11 -6 1) 3 10) 0))
(newline)

(display "3. isolation certified -- one root per interval, strict sign change") (newline)
(display "    cubic roots in ") (display (intervals->string (isolate-roots (list -6 11 -6 1)))) (newline)
(must "cubic isolation certified"              (isolation-ok? (list -6 11 -6 1) (isolate-roots (list -6 11 -6 1))))
(display "    quartic roots in ") (display (intervals->string (isolate-roots (list 4 0 -5 0 1)))) (newline)
(must "quartic isolation certified"            (isolation-ok? (list 4 0 -5 0 1) (isolate-roots (list 4 0 -5 0 1))))
(must "x^5-x isolation certified"              (isolation-ok? (list 0 -1 0 0 0 1) (isolate-roots (list 0 -1 0 0 0 1))))
(must "quartic gives 4 isolating intervals"    (= (length (isolate-roots (list 4 0 -5 0 1))) 4))
(newline)

(display "4. refining an irrational root to a narrow rational bracket") (newline)
(define b (car (isolate-refined (list -2 0 1) (/ 1 1000))))
(display "    a root of x^2-2 lies in ") (display (iv->string b)) (newline)
(must "bracket width below 1/1000"             (< (- (car (cdr b)) (car b)) (/ 1 1000)))
(must "bracket brackets a sign change of x^2-2" (< (* (- (* (car b) (car b)) 2) (- (* (car (cdr b)) (car (cdr b))) 2)) 0))
(newline)

(display "all Sturm checks passed.") (newline)
