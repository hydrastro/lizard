(import "cas/ratfun.lisp")
(define (F p) (display (factorization->string (factor-Q p) "x")) (newline))
(define (G a b) (display (poly->string (poly-gcd a b) "x")) (newline))
(define (P num den) (display (pf->string (partial-fractions num den) "x")) (newline))
(F (list -1 0 1))               ; x^2 - 1
(F (list 2 3 1))                ; x^2 + 3x + 2
(F (list -1 0 0 0 1))           ; x^4 - 1
(F (list 2 0 3 0 1))            ; x^4 + 3x^2 + 2
(F (list 1 1 1 1 1))            ; Phi_5 (irreducible)
(F (list 1 0 -10 0 1))          ; Swinnerton-Dyer (irreducible)
(F (list 1 5 6))                ; 6x^2 + 5x + 1
(F (list -2 0 2))               ; 2x^2 - 2
(F (list (/ -1 4) 0 1))         ; x^2 - 1/4
(F (list -1 0 0 0 0 0 1))       ; x^6 - 1
(G (list -1 0 1) (list 1 -2 1)) ; gcd -> x - 1
(G (list -6 -1 0 1) (list -2 -1 0 1))
(P (list 1) (list -1 0 1))                 ; 1/(x^2-1)
(P (list 0 0 0 1) (list -1 0 1))           ; x^3/(x^2-1)
(P (list 1) (list 0 1 0 1))                ; 1/(x^3+x)
(P (list 3 0 2) (poly-pow (list -1 1) 3))  ; (2x^2+3)/(x-1)^3
