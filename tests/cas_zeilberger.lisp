(import "cas/zeilberger.lisp")
; first-order recurrence for SUM C(n,k)
(define c1 (zb-try (list (list 1 1)) (list (list 1 1) (list -1))
                   (list (list 0 1) (list -1)) (list (list 1) (list 1)) (list (list 1 1) (list -1)) 1 0 0 1))
(display (zb-recurrence->string c1)) (newline)
(display (zb-certificate->string c1)) (newline)
; second-order recurrence for central Delannoy numbers
(define a1d (list (list 1 1) (list -1)))
(define c2 (zb-try (list (list 1 1) (list 1)) a1d
                   (list (list 0 1 1) (list -1) (list -1)) (list (list 1) (list 2) (list 1))
                   (bp-mul a1d (bp-shiftn a1d)) 2 1 1 2))
(display (zb-recurrence->string c2)) (newline)
