; 194-best-approximation.lisp -- best rational approximation under a denominator bound.
;
; Given a rational x and a bound N, find p/q with q <= N minimising |x - p/q|.  The
; optimum is always a continued-fraction convergent or a semiconvergent; the answer is
; certified against an exhaustive O(N) search over all denominators.  This recovers the
; historical record approximations of pi: 22/7 (Archimedes) and 355/113 (Zu Chongzhi's
; Milue), the closest of all fractions with denominator under 16604.  `must` raises on
; failure.

(import "cas/bestapprox.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'bestapprox-check-failed)))
(define pi  (/ 314159265358979 100000000000000))
(define e   (/ 271828182845904 100000000000000))
(define rt2 (/ 14142135623731 10000000000000))

(display "Best rational approximation under a denominator bound") (newline) (newline)

(display "1. approximations of pi as the bound grows") (newline)
(display "    N <= 10  -> ") (display (best-approx->string pi 10))  (newline)
(display "    N <= 99  -> ") (display (best-approx->string pi 99))  (newline)
(display "    N <= 113 -> ") (display (best-approx->string pi 113)) (newline)
(display "    N <= 300 -> ") (display (best-approx->string pi 300)) (newline)
(must "N<=10 gives Archimedes 22/7"  (equal? (best-approx pi 10)  (cons 22 7)))
(must "N<=99 gives 311/99"           (equal? (best-approx pi 99)  (cons 311 99)))
(must "N<=113 gives Milue 355/113"   (equal? (best-approx pi 113) (cons 355 113)))
(must "355/113 still best at N<=300" (equal? (best-approx pi 300) (cons 355 113)))
(newline)

(display "2. other constants") (newline)
(display "    e,   N <= 100 -> ") (display (best-approx->string e 100)) (newline)
(display "    e,   N <= 10  -> ") (display (best-approx->string e 10))  (newline)
(display "    rt2, N <= 5   -> ") (display (best-approx->string rt2 5)) (newline)
(must "e within 100 is 193/71" (equal? (best-approx e 100) (cons 193 71)))
(must "e within 10 is 19/7"    (equal? (best-approx e 10)  (cons 19 7)))
(must "rt2 within 5 is 7/5"    (equal? (best-approx rt2 5) (cons 7 5)))
(newline)

(display "3. every answer certified by exhaustive search") (newline)
(must "pi certified at N=10,99,113,200,300" (and (best-approx-ok? pi 10) (best-approx-ok? pi 99) (best-approx-ok? pi 113) (best-approx-ok? pi 200) (best-approx-ok? pi 300)))
(must "e certified at N=10,50,100"          (and (best-approx-ok? e 10) (best-approx-ok? e 50) (best-approx-ok? e 100)))
(must "rt2 certified at N=5,20,100"         (and (best-approx-ok? rt2 5) (best-approx-ok? rt2 20) (best-approx-ok? rt2 100)))
(newline)

(display "all best-approximation checks passed.") (newline)
