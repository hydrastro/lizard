; The exponential Risch differential equation with a theta1 denominator -- a Laurent solution.  The
; solver of tower2exprde.lisp treats b = bbar * theta1^(-l): a denominator theta1^l reduces
; b' + k u' b = a to bbar' + (k u' - l) bbar = num, the same top-down recursion with the diagonal term j
; shifted to j - l.  With theta1 = e^x and theta2 = exp(e^x) (u' = theta1), the equation
; b' + theta1 b = 1 - theta1^(-1) has the solution b = theta1^(-1) = e^{-x}, a negative power of theta1
; that the polynomial solver cannot reach.  Only pure-power denominators theta1^l are admitted (the
; special part for an exponential monomial); the polynomial case is the l = 0 instance and still routes
; through.  Every result is certified by the exponential derivation D2.
(import "cas/tower2exprde.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define EXP1 (list 'exp (list 0 1)))
(define t1    (list (list (rat-zero) (rat-one)) (list (rat-one))))            ; theta1 = e^x  (u' = theta1)
(define t1inv (list (list (rat-one)) (list (rat-zero) (rat-one))))            ; theta1^(-1) = e^{-x}
(define a (k1-sub (k1-one) t1inv))                                           ; 1 - theta1^(-1)
(display "Exponential RDE with a theta1 denominator (Laurent solution)  [theta1 = e^x, theta2 = exp(e^x)]") (newline)
(define b (exp-rde-laurent a 1 t1 EXP1))
(must "RDE b' + theta1 b = 1 - theta1^(-1) is solvable" (exp-rde-laurent-solvable? a 1 t1 EXP1))
(must "its solution is b = theta1^(-1) = e^{-x} (a NEGATIVE power of theta1)" (tr-equal? (tr-reduce b) (tr-reduce t1inv)))
(must "RDE solution certified: b' + theta1 b = 1 - theta1^(-1)" (exp-rde-check a 1 t1 b EXP1))
(must "the polynomial case (l=0) still routes through: b' + theta1 b = theta1 + theta1^2 gives theta1"
      (tr-equal? (tr-reduce (exp-rde-laurent (k1-add t1 (k1-mul t1 t1)) 1 t1 EXP1)) (tr-reduce t1)))
(newline) (display "all Laurent exponential-RDE checks passed.") (newline)
