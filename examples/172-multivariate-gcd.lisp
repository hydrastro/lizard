; 172-multivariate-gcd.lisp — GCD of bivariate polynomials over Q.
;
; f(x,y) is a list of Q[y] coefficients in x.  The GCD is computed over the field
; Q(y) by the Euclidean algorithm (so no pseudo-division), then cleared of
; y-denominators and made primitive over Q[y] (Gauss's lemma) to land back in Q[x,y].
; A true gcd divides BOTH inputs exactly, so each case is certified by dividing the
; inputs by the result (remainder zero over Q(y)[x] iff it divides over Q[x,y]).
; `must` raises on failure.

(import "cas/mgcd.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'mgcd-check-failed)))
(define x+y (list (list 0 1) (list 1)))
(define x-y (list (list 0 -1) (list 1)))
(define x-1 (list (list -1) (list 1)))
(define x+1 (list (list 1) (list 1)))
(define (chk label f g expect)
  (let ((d (mgcd f g)))
    (display "    ") (display label) (display "  =  ") (display (xy->string d)) (newline)
    (must "divides first input"  (divides? d f))
    (must "divides second input" (divides? d g))
    (must "equals expected gcd"  (equal? d expect))))

(display "Bivariate polynomial GCD over Q") (newline) (newline)

(display "1. common factor (x+y)") (newline)
(chk "gcd(x^2-y^2, (x+y)^2)" (xy-mul x-y x+y) (xy-mul x+y x+y) x+y)
(chk "gcd(x^2-y^2, x-y)"     (xy-mul x-y x+y) x-y x-y)
(chk "gcd((x+y)(x-1), (x+y)(x+1))" (xy-mul x+y x-1) (xy-mul x+y x+1) x+y)
(newline)

(display "2. larger common factor (x+y)(x-1) = x^2 + (y-1)x - y") (newline)
(chk "gcd((x+y)^2(x-1), (x+y)(x-1)^2)"
     (xy-mul (xy-mul x+y x+y) x-1) (xy-mul (xy-mul x+y x-1) x-1)
     (xy-mul x+y x-1))
(newline)

(display "3. coprime inputs -> gcd = 1") (newline)
(must "gcd(x+y, x-y) = 1" (equal? (mgcd x+y x-y) (list (list 1))))
(must "gcd(x-1, x+1) = 1" (equal? (mgcd x-1 x+1) (list (list 1))))
(newline)

(display "all bivariate-GCD checks passed.") (newline)
