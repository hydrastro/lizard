; 201-finite-differences.lisp -- finite-difference calculus and Newton interpolation.
;
; The discrete analogue of calculus.  From values at 0..n the forward difference operator
; builds a table whose leading entries are the Newton coefficients of the unique
; interpolating polynomial P(x) = sum_k (D^k y_0) C(x,k).  Summation is the discrete
; integral: sum_{i=0}^{m-1} P(i) = sum_k (D^k y_0) C(m,k+1) in closed form, and the
; antidifference Q satisfies Q(x+1) - Q(x) = P(x).  Faulhaber's power sums interpolate
; x^p.  Each result is checked independently -- the polynomial reproduces the data, and
; closed-form sums equal brute-force sums.  `must` raises on failure.

(import "cas/findiff.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'findiff-check-failed)))

(display "Finite differences and Newton forward-difference interpolation") (newline) (newline)

(display "1. interpolation recovers the polynomial exactly") (newline)
(display "    0,1,4,9,16        -> ") (display (newton-poly->string (list 0 1 4 9 16))) (newline)
(display "    0,1,8,27,64,125   -> ") (display (newton-poly->string (list 0 1 8 27 64 125))) (newline)
(display "    2,3,8,17,30       -> ") (display (newton-poly->string (list 2 3 8 17 30))) (newline)
(must "x^2 reproduced at all nodes"   (interp-ok? (list 0 1 4 9 16)))
(must "x^3 reproduced at all nodes"   (interp-ok? (list 0 1 8 27 64 125)))
(must "2x^2-x+2 reproduced at nodes"  (interp-ok? (list 2 3 8 17 30)))
(must "Newton coeffs of x^2 data are 0,1,2,0,0" (equal? (newton-coeffs (list 0 1 4 9 16)) (list 0 1 2 0 0)))
(must "interpolant degree is bounded" (degree-ok? (list 2 3 8 17 30)))
(newline)

(display "2. summation is the discrete integral (closed form = brute force)") (newline)
(display "    sum_{i=0}^{9} i^2  = ") (display (power-sum 2 9)) (newline)
(display "    sum_{i=0}^{10} i^3 = ") (display (power-sum 3 10)) (newline)
(must "sum of squares to 9 is 285"   (= (power-sum 2 9) 285))
(must "sum of cubes to 10 is 3025"   (= (power-sum 3 10) 3025))
(must "Faulhaber p=2 verified to n=30" (faulhaber-ok? 2 30))
(must "Faulhaber p=3 verified to n=30" (faulhaber-ok? 3 30))
(must "Faulhaber p=4 verified to n=30" (faulhaber-ok? 4 30))
(must "Faulhaber p=5 verified to n=25" (faulhaber-ok? 5 25))
(must "closed-form sum matches direct sum for 2,3,8,17,30 to m=12" (sum-ok? (list 2 3 8 17 30) 12))
(newline)

(display "3. antidifference: Q with Q(x+1) - Q(x) = P(x)") (newline)
(must "antidifference of x^2 verified"  (antidiff-ok? (list 0 0 1)))
(must "antidifference of x^3 verified"  (antidiff-ok? (list 0 0 0 1)))
(must "antidifference of 2x^2-x+2 verified" (antidiff-ok? (list 2 -1 2)))
(newline)

(display "all finite-difference checks passed.") (newline)
