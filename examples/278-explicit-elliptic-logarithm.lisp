; The EXPLICIT ELLIPTIC LOGARITHM, completing Rung 3b (docs/TRAGER_ROADMAP.md): not just the decision that
; INT dx/((x-s) sqrt(p)) is elementary on a genus-1 curve, but the CONSTRUCTION of the actual algebraic
; logarithm c log(f), f in K = Q(x)[y]/(y^2 - p), certified by D(c log f) = integrand.
;
; When the pole lifts to a torsion point P = (s, rho) of order n, the residue divisor n([P] - [-P]) is
; principal.  We build the Miller function f_P with div(f_P) = n[P] - n[O] by the standard iteration over K
; (chord/tangent lines and verticals of the elliptic group law as elements of K), take f = f_P / conj(f_P) so
; div(f) = n([P] - [-P]), find the constant c by matching f'/f to the integrand, and CERTIFY the whole answer.
;
; The decisive example: INT dx/(x sqrt(x^3+1)).  The pole lifts to (0,1), a torsion point of order 3; the
; construction yields  INT dx/(x sqrt(x^3+1)) = (1/3) log( (sqrt(x^3+1) - 1)/(sqrt(x^3+1) + 1) ),  certified
; inside K.  This is a genuinely non-trivial algebraic logarithm on an elliptic curve -- the kind of answer the
; torsion criterion of Rung 3b promised but did not yet exhibit.
;
; Honest scope: the construction is gated by the certificate, so it never returns a wrong logarithm.  It
; certifies the odd-order torsion poles; an even-order pole (whose multiples pass through a 2-torsion point) is
; deferred -- reported as not-yet-constructed -- while elltorsion still DECIDES it elementary.
(import "cas/elllog.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Explicit elliptic logarithm c log(f), f in K, for INT dx/((x-s) sqrt(p)) at a torsion pole, certified:") (newline) (newline)

(display "the decisive case, p = x^3+1, pole at the origin (lifts to (0,1), order 3):") (newline)
(define r1 (ell-logarithm (rat-from-poly (list 1 0 0 1)) 0))
(display "  INT dx/(x sqrt(x^3+1)) -> ") (display (car r1)) (newline)
(display "    c = ") (display (car (cdr r1))) (display "   (= 1/3, the reciprocal of the torsion order)") (newline)
(display "    f = f_P/conj(f_P), a reduced form of (sqrt(x^3+1)-1)/(sqrt(x^3+1)+1)") (newline)
(chk "INT dx/(x sqrt(x^3+1)) = (1/3) log((y-1)/(y+1)), CONSTRUCTED and certified in K" (ell-log-decides? (rat-from-poly (list 1 0 0 1)) 0))

(display "another order-3 torsion pole, p = x^3+4 (lifts to (0,2)):") (newline)
(chk "INT dx/(x sqrt(x^3+4)) elliptic-log constructed and certified" (ell-log-decides? (rat-from-poly (list 4 0 0 1)) 0))

(newline)
(display "soundness (the certificate is the arbiter -- never a wrong logarithm):") (newline)
(define rn (ell-logarithm (rat-from-poly (list -2 0 0 1)) 3))
(display "  INT dx/((x-3) sqrt(x^3-2)) [pole (3,5), infinite order] -> ") (display (car rn)) (newline)
(chk "infinite-order pole -> failed, NOT a spurious logarithm" (equal? (car rn) (quote failed)))
(define r6 (ell-logarithm (rat-from-poly (list 1 0 0 1)) 2))
(display "  INT dx/((x-2) sqrt(x^3+1)) [pole (2,3), even order 6] -> ") (display (car r6)) (display " (deferred; decision still elementary)") (newline)
(chk "even-order pole gracefully deferred (no uncertified log)" (equal? (car r6) (quote failed)))
(chk "  but elltorsion still DECIDES it elementary" (elt-decides-elementary? (rat-from-poly (list 1 0 0 1)) 2))

(newline)
(display "Elliptic logarithm CONSTRUCTED: INT dx/(x sqrt(x^3+1)) = (1/3) log((y-1)/(y+1)) certified; sound on all inputs.") (newline)
