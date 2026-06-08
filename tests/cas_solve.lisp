(import "cas/solve.lisp")
(import "cas/resultant.lisp")
(define (S p) (display (solutions->string (solve-poly p))) (newline) (display "--") (newline))
(define (R label v) (display label) (display " = ") (display v) (newline))
(S (list 6 -5 1))        ; x^2-5x+6
(S (list -2 0 1))        ; x^2-2
(S (list 1 0 1))         ; x^2+1
(S (list -1 1 1))        ; x^2+x-1
(S (list -2 0 0 1))      ; x^3-2
(S (list 1 0 -10 0 1))   ; x^4-10x^2+1
(R "res(x^2-1,x^2-4)" (resultant (list -1 0 1) (list -4 0 1)))
(R "disc(x^2-2)" (discriminant (list -2 0 1)))
(R "disc(x^3-2)" (discriminant (list -2 0 0 1)))
(R "realroots(x^4-10x^2+1)" (count-real-roots (list 1 0 -10 0 1)))
(R "RT-res(1/(x^2-1))" (poly->string (rt-resultant (list 1) (list -1 0 1)) "z"))
