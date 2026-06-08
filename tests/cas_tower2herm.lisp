(import "cas/tower2herm.lisp")
(define EXP1 (list 'exp (list 0 1)))
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))
(define A1 (list Dth2))
(define D1 (list (tr-zero) (tr-zero) (t2-trone)))
(define H1 (h2-hermite A1 D1 Dth2 EXP1))
(display (h2-equal? (car (car H1)) (list (k1-neg (k1-one))))) (newline)   ; g num = -1
(display (h2-zero? (car (cdr H1)))) (newline)                              ; remainder zero
(display (h2tr-equal? (h2tr-add (h2tr-deriv (car (car H1)) (car (cdr (car H1))) Dth2 EXP1)
                                (list (car (cdr H1)) (car (cdr (cdr H1))))) (list A1 D1))) (newline)
