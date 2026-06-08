(import "cas/integrate.lisp")
(define (I num den) (display (integral->string (integrate-rational num den) "x")) (newline))
(I (list 0 0 1) (list 1))                    ; x^2 -> x^3/3
(I (list 1) (list -1 0 1))                   ; 1/(x^2-1)
(I (list 0 0 0 1) (list -1 0 1))             ; x^3/(x^2-1)
(I (list 1) (poly-pow (list -1 1) 2))        ; 1/(x-1)^2
(I (list 1) (list 1 0 1))                    ; 1/(x^2+1) -> arctan(x)
(I (list 1) (list 1 1 1))                    ; 1/(x^2+x+1)
(I (list 1) (list 0 1 0 1))                  ; 1/(x^3+x)
(I (list 4 5) (poly-mul (list -1 1) (list 4 0 1)))  ; (5x+4)/((x-1)(x^2+4))
(I (list 1) (list -2 0 1))                   ; 1/(x^2-2) -> cannot
