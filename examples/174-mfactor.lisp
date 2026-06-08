; 174-mfactor.lisp — factorization of bivariate polynomials over Q into irreducibles,
; by evaluation + y-adic Hensel lifting + recombination.
;
; For a squarefree, primitive, monic-in-x f(x,y): pick a shift s so f(x,s) keeps full
; x-degree and is squarefree; factor that univariate polynomial over Q; Hensel-lift the
; factorization from mod (y-s) up to mod (y-s)^N with N > deg_y f; then recombine, the
; true factors being subsets whose product divides f exactly (mgcd/divides?).  The full
; result is gated by RECONSTRUCTION: the product of the returned factors (with their
; multiplicities, after a squarefree split) must equal f, so a wrong factorization can
; never be reported.  `must` raises on failure.

(import "cas/mfactor.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'mfactor-check-failed)))
; mgcd "xy" layout: a polynomial is a list of Q[y] coefficients in x (low to high)
(define x+y (list (list 0 1) (list 1)))
(define x-y (list (list 0 -1) (list 1)))
(define x+1 (list (list 1) (list 1)))
(define x-1 (list (list -1) (list 1)))
(define x2+y (list (list 0 1) (list) (list 1)))        ; x^2 + y       (irreducible)
(define x2-2 (list (list -2) (list) (list 1)))         ; x^2 - 2       (irreducible over Q)

(define (check label f n)
  (let ((facts (factor-bivariate f)))
    (display "  ") (display label) (display " = ") (display (factor-bivariate->string facts)) (newline)
    (must "    reconstruction certificate" (factor-ok? f facts))
    (must "    distinct-factor count" (= (length facts) n))))

(display "Bivariate factorization over Q (Hensel + recombination)") (newline) (newline)

(display "1. products of distinct linear factors") (newline)
(check "x^2 - y^2"           (xy-mul x+y x-y) 2)
(check "x^2 - 1"             (xy-mul x+1 x-1) 2)
(check "(x-y)(x+y)(x+1)"     (xy-mul (xy-mul x+y x-y) x+1) 3)
(newline)

(display "2. mixed-degree factors") (newline)
(check "(x+1)(x^2+y)"        (xy-mul x+1 x2+y) 2)
(newline)

(display "3. irreducible polynomials (returned whole)") (newline)
(check "x^2 + y"             x2+y 1)
(check "x^2 - 2"             x2-2 1)
(newline)

(display "4. repeated factors (squarefree split then factor)") (newline)
(check "(x+y)^2 (x-1)"       (xy-mul (xy-mul x+y x+y) x-1) 2)
(newline)

(display "all bivariate-factorization checks passed.") (newline)
