(import "cas/rt.lisp")
(define (L p q) (display (rt-log->string (rt-log-part p q))) (newline))
(define (C p q) (display (rt-certificate p q (rt-log-part p q))) (newline))
(L (list 1) (list -1 0 1))                         ; 1/(x^2-1)
(L (list 0 2) (list -1 0 1))                       ; 2x/(x^2-1)
(L (list 1) (poly-mul (list -1 1) (list -2 1)))    ; 1/((x-1)(x-2))
(C (list 1) (list -2 0 1))                         ; 1/(x^2-2) certified
(C (list 1) (list 1 0 1))                          ; 1/(x^2+1) certified
(C (list 1) (list -2 0 0 1))                       ; 1/(x^3-2) certified
