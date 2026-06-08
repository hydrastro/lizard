; INT P(x)/sqrt(R) dx for an ARBITRARY polynomial numerator P and monic quadratic R = x^2 + b x + c,
; extending algfunc's linear-numerator radical integrals.  The reduction
;     INT P/sqrt(R) dx = Q(x) sqrt(R) + lambda INT dx/sqrt(R)
; gives the triangular polynomial identity P = Q' R + Q R'/2 + lambda, solved in one descending pass, and
; the answer is certified by differentiation inside the algebraic function field K = Q(x)[sqrt(R)].
(import "cas/algfuncint.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define R1 (list 1 0 1))   (define R2 (list 5 2 1))
(display "INT (x^2+1)/sqrt(x^2+1) dx: Q, lambda = ") (display (afi-build (list 1 0 1) R1))
(display "  => (x/2)sqrt(x^2+1) + (1/2) log(...)") (newline) (newline)
(must "INT x^2/sqrt(x^2+1) dx certified in Q(x)[sqrt R]"        (int-poly-sqrt-certify (list 0 0 1) R1))
(must "INT x^3/sqrt(x^2+1) dx certified"                        (int-poly-sqrt-certify (list 0 0 0 1) R1))
(must "INT (3x^4 - 2x^2 + 1)/sqrt(x^2+1) dx certified"          (int-poly-sqrt-certify (list 1 0 -2 0 3) R1))
(must "INT (2x^3 + x)/sqrt(x^2+2x+5) dx certified"              (int-poly-sqrt-certify (list 0 1 0 2) R2))
(must "INT x^5/sqrt(x^2+2x+5) dx certified"                     (int-poly-sqrt-certify (list 0 0 0 0 0 1) R2))
(newline) (display "polynomial-numerator radical integrals certified in the algebraic function field.") (newline)
