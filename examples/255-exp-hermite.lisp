; Hermite reduction for an exponential second monomial at height two.  For theta2 = exp(u) with
; D2 theta2 = u' theta2, an integrand A/D whose denominator is coprime to theta2 but not squarefree in
; theta2 is reduced to a rational part plus a remainder with squarefree denominator.  The algorithm is the
; primitive Hermite reduction of tower2herm.lisp with the exponential derivation t2e-deriv in place of the
; primitive one; it is purely rational (gcd, division, Bezout over K1[theta2]) and does not use the
; Rothstein-Trager resultant.  With theta1 = e^x and theta2 = exp(e^x) (u' = theta1),
;     INT -theta1 theta2/(theta2 - 1)^2 dx = 1/(theta2 - 1)   (up to a constant),
; whose squarefree remainder vanishes -- the antiderivative is purely rational.  The reduction is computed
; once and certified by differentiating the rational part with D2 and matching A/D (the remainder being
; zero, this is the whole certificate).
(import "cas/tower2exphermite.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define EXP1 (list 'exp (list 0 1)))
(define t1 (list (list (rat-zero) (rat-one)) (list (rat-one))))             ; theta1 = e^x  (u' = theta1)
(define A (list (k1-zero) (k1-neg t1)))                                      ; -theta1 theta2
(define D (list (k1-one) (k1-iscale -2 (k1-one)) (k1-one)))                 ; (theta2 - 1)^2
(display "INT -theta1 theta2/(theta2-1)^2 dx   [theta2 = exp(e^x)]") (newline)
(define H (t2e-hermite A D t1 EXP1))
(define g (car H))
(must "Hermite lowers the (theta2-1)^2 denominator to degree 1" (= (h2-deg (car (cdr g))) 1))
(must "the squarefree remainder vanishes: the integral is purely rational" (h2-zero? (car (cdr H))))
(must "CERTIFIED: D2(rational part) = integrand A/D over K1[theta2]"
      (h2tr-equal? (t2eh-deriv (car g) (car (cdr g)) t1 EXP1) (list A D)))
(newline) (display "exponential Hermite reduction certified.") (newline)
