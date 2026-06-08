; RUNG 5, the fusion step: a NESTED LOGARITHM over the ALGEBRAIC base.  This combines the nested-log tower
; (nestlog.lisp) with the entangled algebraic logarithm (alglog.lisp): the base is the algebraic field
; K = Q(x)[y]/(y^n - g) (e.g. y = sqrt x), and over it we build t1 = log(w) for a field element w, then
; t2 = log(t1) = log(log(w)) -- the first tower that is BOTH nested AND over the algebraic base
; (docs/TRAGER_ROADMAP.md, Rung 5).
;
; From alglog, t1 = log(w) has t1' = w'/w, a FIELD element of K.  Then t2 = log(t1) has t2' = t1'/t1, which
; carries t1 in its DENOMINATOR (the nestlog structure) with a field element in its numerator (the alglog
; structure).  A K(t1) element is a rational function in t1 with sefield-element coefficients; the cleanest
; integral is INT (t1'/t1) dx = t2 = log(log(w)), certified by differentiating in the tower.
(import "cas/nestalg.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "A nested logarithm over the algebraic base: t1 = log(w), t2 = log(log(w)), w a field element of K.") (newline) (newline)

(define p (list 0 1))   ; y^2 = x, so y = sqrt x
(define w (sf-set (sf-set (sf-zeros 2) 0 (rat-one)) 1 (rat-one)))   ; w = sqrt x + 1
(define tp (na-t1prime p 2 w))   ; t1' = w'/w, a field element of K
(display "the inner algebraic logarithm t1 = log(sqrt x + 1) has derivative t1' = w'/w = ") (display tp) (newline)
(chk "t1' is a genuine field element of K (t1' w = w')" (sf-equal? (sf-product p 2 tp w) (sf-deriv p 2 w)))

(display "the outer monomial t2 = log(t1) = log(log(sqrt x + 1)) has derivative t2' = t1'/t1:") (newline)
(define E (na-answer-t2 2))   ; E = t2
(define B (na-deriv p 2 tp E))
(chk "d/dx(log(log(sqrt x + 1))) = (w'/w)/log(sqrt x + 1) = t2'" (na-q-eq? p 2 (na-tcoeff p 2 B 0) (na-t2prime p 2 tp)))

(display "the nested-log-over-algebraic integral INT (w'/w)/log(w) dx = log(log(sqrt x + 1)):") (newline)
(chk "INT (w'/w)/log(sqrt x + 1) dx = log(log(sqrt x + 1)), certified in the tower" (na-certify p 2 tp (na-answer-t2 2) (na-integrand p 2 tp)))

(display "a higher case, INT 2 log(log w) (w'/w)/log(w) dx = (log(log w))^2:") (newline)
(define E2 (list (na-q-zero 2) (na-q-zero 2) (na-q-one 2)))   ; t2^2
(chk "d/dx((log log w)^2) = 2 log(log w) (w'/w)/log(w), certified" (na-certify p 2 tp E2 (na-deriv p 2 tp E2)))

(display "the same over the cube-root base y^3 = x, w = x^(1/3) + 1:") (newline)
(define p3 (list 0 1))
(define w3 (sf-set (sf-set (sf-zeros 3) 0 (rat-one)) 1 (rat-one)))
(define tp3 (na-t1prime p3 3 w3))
(chk "INT (w'/w)/log(w) dx = log(log(x^(1/3) + 1)), n = 3" (na-certify p3 3 tp3 (na-answer-t2 3) (na-integrand p3 3 tp3)))

(newline)
(display "A nested logarithm over the algebraic base: the inner log's derivative w'/w is a field element of K,") (newline)
(display "the outer carries log(w) in its denominator, and INT (w'/w)/log(w) = log(log w) is certified -- the") (newline)
(display "fusion of nested towers with the algebraic base.") (newline)
