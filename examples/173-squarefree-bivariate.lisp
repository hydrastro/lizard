; 173-squarefree-bivariate.lisp — squarefree factorization of bivariate polynomials
; over Q via Yun's algorithm on the bivariate GCD.
;
; Yun's method separates repeated factors using gcd(f, df/dx), producing pairwise
; coprime, squarefree factors a_1, a_2, ... with f = prod a_i^i.  Each step is a
; bivariate gcd plus an exact bivariate division.  Each result is certified two ways:
; reconstruction (prod a_i^i must equal f up to a constant) and squarefreeness of every
; factor (gcd(a_i, a_i') constant).  `must` raises on failure.

(import "cas/msqfree.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'sqfree-check-failed)))
(define x+y (list (list 0 1) (list 1)))
(define x-y (list (list 0 -1) (list 1)))
(define x-1 (list (list -1) (list 1)))
(define (chk label f expect)
  (let ((fac (sqfree f)))
    (display "    ") (display label) (display "  =  ") (display (sqfree->string fac)) (newline)
    (must "reconstruction and squarefreeness certified" (sqfree-ok? f fac))
    (must "matches expected factorization" (equal? fac expect))))

(display "Bivariate squarefree factorization over Q") (newline) (newline)

(display "1. one repeated factor") (newline)
(chk "(x+y)^2 (x-1)" (xy-mul (xy-mul x+y x+y) x-1) (list (cons x-1 1) (cons x+y 2)))
(chk "(x+y)^3"       (xy-mul (xy-mul x+y x+y) x+y) (list (cons x+y 3)))
(newline)

(display "2. already squarefree") (newline)
(chk "x^2 - y^2"     (xy-mul x-y x+y) (list (cons (xy-mul x-y x+y) 1)))
(newline)

(display "3. two distinct multiplicities") (newline)
(chk "(x-1)^2 (x+y)^3" (xy-mul (xy-mul x-1 x-1) (xy-mul (xy-mul x+y x+y) x+y)) (list (cons x-1 2) (cons x+y 3)))
(newline)

(display "4. (x^2-y^2)^2 -> radical x^2-y^2 at multiplicity 2") (newline)
(chk "(x^2-y^2)^2" (xy-mul (xy-mul x-y x+y) (xy-mul x-y x+y)) (list (cons (xy-mul x-y x+y) 2)))
(newline)

(display "all bivariate squarefree-factorization checks passed.") (newline)
