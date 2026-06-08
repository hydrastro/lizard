; A DECISION procedure for the elementarity of INT P(x) e^{g(x)} dx (P, g polynomials over Q, deg g >= 1) -- the
; first genuine DECIDER in the system.  Every prior tower module is a constructive certifier: it finds an
; antiderivative and proves it correct.  This module also PROVES NON-ELEMENTARITY when no elementary
; antiderivative exists, by Liouville's theorem (docs/TRAGER_ROADMAP.md, the summit).
;
; Liouville (exponential case): INT P e^g dx is elementary iff there is a rational R with R' + g' R = P; then the
; antiderivative is R e^g.  For polynomial data the solution R, if any, is a polynomial of degree exactly
; deg(P) - deg(g) + 1 (a degree argument, since deg(g' R) > deg(R')).  If that degree is negative the only
; candidate is R = 0, forcing P = 0; otherwise R's coefficients solve an exact linear system.  A solution is a
; PROOF of elementarity (verify R' + g' R = P, so (R e^g)' = P e^g); an inconsistent system is a PROOF of
; non-elementarity.  The classic special functions (erf from INT e^{x^2}, Ei from INT e^x/x) come out as
; proven-non-elementary.
(import "cas/liouville.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "A decision procedure for INT P(x) e^{g(x)} dx: elementary (with the antiderivative) or PROVEN not.") (newline) (newline)

(display "elementary integrals -- the decider finds R with R' + g' R = P, so INT = R e^g:") (newline)
(display "  INT x e^{x^2} dx: ") (display (lv-decide (list 0 1) (list 0 0 1))) (display "  (R = 1/2, so INT = (1/2) e^{x^2})") (newline)
(chk "INT x e^{x^2} = (1/2) e^{x^2}, R certified" (if (equal? (car (lv-decide (list 0 1) (list 0 0 1))) (quote elementary)) (lv-certify (list 0 1) (list 0 0 1) (car (cdr (lv-decide (list 0 1) (list 0 0 1))))) #f))
(display "  INT x e^x dx: ") (display (lv-decide (list 0 1) (list 0 1))) (display "  (R = x - 1, so INT = (x-1) e^x)") (newline)
(chk "INT x e^x = (x-1) e^x, R certified" (if (equal? (car (lv-decide (list 0 1) (list 0 1))) (quote elementary)) (lv-certify (list 0 1) (list 0 1) (car (cdr (lv-decide (list 0 1) (list 0 1))))) #f))
(display "  INT x^2 e^{x^3} dx: ") (display (lv-decide (list 0 0 1) (list 0 0 0 1))) (display "  (R = 1/3)") (newline)
(chk "INT x^2 e^{x^3} = (1/3) e^{x^3}, R certified" (if (equal? (car (lv-decide (list 0 0 1) (list 0 0 0 1))) (quote elementary)) (lv-certify (list 0 0 1) (list 0 0 0 1) (car (cdr (lv-decide (list 0 0 1) (list 0 0 0 1))))) #f))

(display "PROVEN non-elementary -- the decider shows no rational R can exist:") (newline)
(display "  INT e^{x^2} dx (the error function erf): ") (display (lv-decide (list 1) (list 0 0 1))) (newline)
(chk "INT e^{x^2} dx is PROVEN non-elementary" (equal? (car (lv-decide (list 1) (list 0 0 1))) (quote non-elementary)))
(display "  INT e^{x^3} dx: ") (display (lv-decide (list 1) (list 0 0 0 1))) (newline)
(chk "INT e^{x^3} dx is PROVEN non-elementary" (equal? (car (lv-decide (list 1) (list 0 0 0 1))) (quote non-elementary)))
(display "  INT x e^{x^3} dx: ") (display (lv-decide (list 0 1) (list 0 0 0 1))) (newline)
(chk "INT x e^{x^3} dx is PROVEN non-elementary" (equal? (car (lv-decide (list 0 1) (list 0 0 0 1))) (quote non-elementary)))
(display "  INT e^x / x dx (the exponential integral Ei): ") (display (lv-decide-exp-over-x)) (newline)
(chk "INT e^x/x dx is PROVEN non-elementary (Ei)" (equal? (car (lv-decide-exp-over-x)) (quote non-elementary)))

(newline)
(display "The first genuine decider: by Liouville's theorem it returns an elementary antiderivative R e^g WITH a") (newline)
(display "certificate when one exists, and a PROOF of non-elementarity (no rational R) when none does -- the erf and") (newline)
(display "Ei integrals among the proven-impossible.") (newline)
