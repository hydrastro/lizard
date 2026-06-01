(import "cas/ode.lisp")
(define N 9)
(display (ode-series (list (list -1) (list 1)) (list) (list 1) N)) (newline)               ; exp
(display (ode-series (list (list 1) (list 0) (list 1)) (list) (list 0 1) N)) (newline)      ; sin
(display (ode-series (list (list 1) (list 0) (list 1)) (list) (list 1 0) N)) (newline)      ; cos
(display (ode-series (list (list -1) (list 1 -1)) (list) (list 1) N)) (newline)             ; 1/(1-x)
(display (ode-series (list (list 0 -1) (list 0) (list 1)) (list) (list 1 0) N)) (newline)   ; Airy
(display (ode-series (list (list 4) (list 0 -2) (list 1)) (list) (list 1 0) N)) (newline)   ; Hermite H2
