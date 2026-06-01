; 170-rational-limits.lisp — exact limits of rational functions at any finite point
; and at infinity, by local Taylor expansion.
;
; lim_{x->a} p/q is found by expanding p and q about a (exact Taylor coefficients via
; synthetic division) and comparing orders of vanishing: ordinary points give p(a)/q(a),
; removable 0/0 singularities give the ratio of leading nonzero coefficients (an exact
; L'Hopital), and poles are infinite.  At infinity the degrees decide.  `must` raises.

(import "cas/ratlimit.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'limit-check-failed)))

(display "Exact limits of rational functions") (newline) (newline)

(display "1. removable singularities (0/0 resolved exactly)") (newline)
(must "lim x->1 (x^2-1)/(x-1) = 2"            (= (ratfun-limit (list -1 0 1) (list -1 1) 1) 2))
(must "lim x->2 (x-2)/(x^2-4) = 1/4"          (= (ratfun-limit (list -2 1) (list -4 0 1) 2) (/ 1 4)))
(must "lim x->3 (x^2-9)/(x^2-5x+6) = 6"       (= (ratfun-limit (list -9 0 1) (list 6 -5 1) 3) 6))
(must "lim x->0 (x^3-x)/(x^2-x) = 1"          (= (ratfun-limit (list 0 -1 0 1) (list 0 -1 1) 0) 1))
(newline)

(display "2. poles") (newline)
(must "lim x->1 (x-1)/(x-1)^2 is infinite"    (equal? (ratfun-limit (list -1 1) (list 1 -2 1) 1) 'infinite))
(must "lim x->0 1/x^2 is infinite"            (equal? (ratfun-limit (list 1) (list 0 0 1) 0) 'infinite))
(newline)

(display "3. ordinary points") (newline)
(must "lim x->2 (x^2+1)/(x+1) = 5/3"          (= (ratfun-limit (list 1 0 1) (list 1 1) 2) (/ 5 3)))
(newline)

(display "4. limits at infinity") (newline)
(must "lim x->inf (2x^2+1)/(x^2+x) = 2"       (= (ratfun-limit-inf (list 1 0 2) (list 0 1 1)) 2))
(must "lim x->inf (x+1)/(x^2+1) = 0"          (= (ratfun-limit-inf (list 1 1) (list 1 0 1)) 0))
(must "lim x->inf (x^3)/(x^2+1) is infinite"  (equal? (ratfun-limit-inf (list 0 0 0 1) (list 1 0 1)) 'infinite))
(must "lim x->inf (3x^2-x)/(2x^2+5) = 3/2"    (= (ratfun-limit-inf (list 0 -1 3) (list 5 0 2)) (/ 3 2)))
(newline)

(display "local expansion of (x^2-1)/(x-1) at x=1:") (newline)
(display "  numerator Taylor coeffs at 1: ") (display (taylor-at (list -1 0 1) 1)) (display "  (= (x-1)(x-1) + 2(x-1) ...)") (newline)
(display "all rational-limit checks passed.") (newline)
