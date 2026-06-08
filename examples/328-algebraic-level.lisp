; The ALGEBRAIC LEVEL for the recursive tower: a level (alg n a) where theta is an algebraic element satisfying
; theta^n = a, extending the recursive derivation and element algebra to algebraic extensions -- the first
; non-transcendental level type in the height-n Risch recursion (docs/TRAGER_ROADMAP.md, the summit).
;
; From theta^n = a, theta' = (a'/(n a)) theta = w theta, so the derivation is DIAGONAL like the exponential level
; but with rate w = a'/(n a); the structural difference is the algebra, where theta-degree stays below n because
; theta^n reduces to a.  This implements the quadratic case theta = sqrt(a).
(import "cas/rischtoweralg.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define t1a (list (list (quote alg) 2 (rat-from-poly (list 0 1)))))   ; theta = sqrt(x)

(display "The algebraic level theta^2 = a: a diagonal derivation with rate w = a'/(2a), algebra reducing theta^2 = a.") (newline) (newline)

(display "the derivative of sqrt(x): D(sqrt x) = (1/(2x)) sqrt x = 1/(2 sqrt x):") (newline)
(define sx (list (rat-zero) (rat-one)))
(define dsx (tea-deriv t1a 1 sx))
(chk "D(sqrt x) has theta-coefficient 1/(2x) (i.e. equals 1/(2 sqrt x))" (rat-equal? (te-acoeff 1 dsx 1) (rat-make (list 1) (list 0 2))))

(display "the algebra reduces theta^2 to a: (sqrt x)^2 = x:") (newline)
(define sq (tea-sq t1a 1 sx))
(chk "(sqrt x)^2 = x" (if (rat-equal? (te-acoeff 1 sq 0) (rat-from-poly (list 0 1))) (rat-equal? (te-acoeff 1 sq 1) (rat-zero)) #f))

(display "a mixed element: D(x + sqrt x) = 1 + 1/(2 sqrt x):") (newline)
(define xsx (list (rat-from-poly (list 0 1)) (rat-one)))
(define dxsx (tea-deriv t1a 1 xsx))
(chk "D(x + sqrt x): rational part 1, theta part 1/(2x)" (if (rat-equal? (te-acoeff 1 dxsx 0) (rat-from-poly (list 1))) (rat-equal? (te-acoeff 1 dxsx 1) (rat-make (list 1) (list 0 2))) #f))

(display "the defining relation is consistent: differentiating theta^2 = a gives D(theta^2) = D(a) = 1:") (newline)
(define asq (list (rat-from-poly (list 0 1)) (rat-zero)))
(chk "D(theta^2) = D(a) = 1" (rat-equal? (te-acoeff 1 (tea-deriv t1a 1 asq) 0) (rat-from-poly (list 1))))

(display "the rate w = a'/(2a) for a = x is 1/(2x):") (newline)
(chk "w = 1/(2x)" (rat-equal? (tea-w t1a 1) (rat-make (list 1) (list 0 2))))

(newline)
(display "The algebraic level adds sqrt-type extensions to the recursive tower: a diagonal derivation with rate") (newline)
(display "a'/(2a) and an algebra that reduces theta^2 = a, consistent with the defining relation -- the first") (newline)
(display "non-transcendental level in the height-n Risch recursion.") (newline)
