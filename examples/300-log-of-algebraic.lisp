; RUNG 5: the ENTANGLED PRIMITIVE tower -- the LOGARITHM OF AN ALGEBRAIC function, t = log(w) with w a field
; element of K = Q(x)[y]/(y^n - g).  The companion of algexp.lisp (exp of an algebraic argument): here the
; primitive monomial's derivative t' = w'/w lives in K (via the field inverse from senorm.lisp), not merely as a
; rational function as in mixedlog/mixedlogn (docs/TRAGER_ROADMAP.md, Rung 5).
;
; We integrate INT (P_1 t + P_0) dx with P_1, P_0 field elements and t = log(w).  An element of the tower K(t)
; is sum_i C_i t^i; the derivation d/dx (sum C_i t^i) = sum (C_i' + (i+1) C_{i+1} t') t^i has t' a FIELD element,
; so C_{i+1} t' is a full field product and the y-power sectors COUPLE (as in algexp).  The answer to a degree-1
; input is Q_2 t^2 + Q_1 t + Q_0 with Q_2' = 0, 2 Q_2 t' + Q_1' = P_1, Q_1 t' + Q_0' = P_0; the coupled system is
; solved by requiring the residual to vanish at sample points, and certified by differentiating in K(t).
(import "cas/alglog.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Logarithm of an algebraic function: INT B dx in the tower K(t), t = log(w) with w algebraic.") (newline) (newline)

(define p (list 0 1))   ; g = x, n = 2, so y = sqrt x
(define y (sf-y 2))
(define w (sf-set (sf-set (sf-zeros 2) 0 (rat-one)) 1 (rat-one)))   ; w = sqrt x + 1
(define tp (al-tprime p 2 w))   ; t' = w'/w, a FIELD element
(display "t = log(sqrt x + 1); its derivative t' = w'/w = ") (display tp) (display "  (a field element of K)") (newline)
(chk "t' is a genuine field element: t' * w = w'" (sf-equal? (sf-product p 2 tp w) (sf-deriv p 2 w)))
(newline)

(display "the pure entangled logarithm INT (w'/w) dx = log(sqrt x + 1):") (newline)
(define Q (list (sf-zeros 2) (sf-one 2)))   ; Q = 1 * t
(define B (al-deriv p 2 tp Q))
(display "  the integrand d/dx(log w) = t' = ") (display B) (newline)
(chk "the constructive identity INT (w'/w) dx = log(w) certifies in K(t)" (al-certify p 2 tp Q B))
(define Qsol (al-solve p 2 tp B 0))
(display "  al-solve recovers Q = ") (display Qsol) (display "  = log(sqrt x + 1)") (newline)
(chk "the solver recovers Q = 1 . t (the logarithm itself)" (if (equal? Qsol (quote none)) #f (al-eq? p 2 Qsol Q)))
(define res (al-integrate p 2 tp B 1))
(chk "the top-level integrator returns the elementary answer" (equal? (car res) (quote elementary)))

(display "the genuine t^2 case, INT (log w) . (w'/w) dx = (1/2)(log w)^2:") (newline)
(define Qt2 (list (sf-zeros 2) (sf-zeros 2) (sf-set (sf-zeros 2) 0 (rat-make (list 1) (list 2)))))   ; (1/2) t^2
(define Bt2 (al-deriv p 2 tp Qt2))
(chk "the solver recovers Q = (1/2)(log w)^2 (answer one t-degree higher)" (if (equal? (al-solve p 2 tp Bt2 0) (quote none)) #f (al-eq? p 2 (al-solve p 2 tp Bt2 0) Qt2)))

(display "the cube-root argument t = log(x^(1/3) + 1) on y^3 = x:") (newline)
(define p3 (list 0 1))   ; g = x, n = 3, y = x^(1/3)
(define w3 (sf-set (sf-set (sf-zeros 3) 0 (rat-one)) 1 (rat-one)))   ; x^(1/3) + 1
(define tp3 (al-tprime p3 3 w3))
(display "  t' = w'/w = ") (display tp3) (newline)
(define Q3 (list (sf-zeros 3) (sf-one 3)))
(define B3 (al-deriv p3 3 tp3 Q3))
(chk "INT (w'/w) dx = log(x^(1/3) + 1) recovered, n = 3" (if (equal? (al-solve p3 3 tp3 B3 0) (quote none)) #f (al-eq? p3 3 (al-solve p3 3 tp3 B3 0) Q3)))

(newline)
(display "Log of an algebraic function: t' = w'/w lives in K, so the tower system couples the y-power sectors --") (newline)
(display "the entangled primitive companion of exp(sqrt x), solved and certified inside K(t).") (newline)
