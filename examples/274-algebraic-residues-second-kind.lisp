; RUNG 1 of the Trager-Bronstein algebraic-integration climb (see docs/TRAGER_ROADMAP.md): the RESIDUES of a
; differential on the hyperelliptic curve y^2 = p, and the one decision residues alone settle -- SECOND KIND
; (all residues zero, hence elementary) versus THIRD KIND (nonzero residues, needing the divisor/torsion rung).
;
; A differential f dx with f = u(x) + v(x) y in K = Q(x)[y]/(y^2 - p) has, over a fibre x = s with p(s) != 0,
; two places (s, +sqrt(p(s))) and (s, -sqrt(p(s))).  The v(x) y part contributes residues res(v)*(+/- sqrt(p(s)))
; -- a CONJUGATE PAIR summing to zero -- while the u(x) part contributes res(u) at BOTH places, summing to
; 2 res(u).  So the finite residue obstruction comes only from the u-part's simple poles; the 1/sqrt(p) (pure
; v y) integrands with simple non-branch poles are automatically residue-free.
;
; This rung classifies that structure exactly and, for the case it owns -- a 1/sqrt(p) integrand with a
; POLYNOMIAL numerator -- delegates to hyperell, which decides elementarity and certifies the answer inside K.
; It honestly reports a u-part simple pole as third-kind (a genuine nonzero residue, NOT elementary), and a
; higher-order pole as not-handled.  Nothing is guessed: an elementary verdict is only the certified hyperell one.
(import "cas/algresidue.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define p3 (list 1 0 0 1))            ; x^3 + 1  (elliptic, genus 1)

(display "Residues on y^2 = x^3+1, and the second-kind / third-kind decision:") (newline) (newline)

(display "cases this rung owns (1/sqrt(p) with polynomial numerator -> certified via hyperell):") (newline)
(define rA (ar-integrate-1oversqrt (rat-zero) (rat-make (list 1) p3) p3))
(display "  INT dx/sqrt(x^3+1) -> ") (display (car rA)) (newline)
(chk "INT dx/sqrt(x^3+1) classified non-elementary (first-kind remainder)" (equal? (car rA) (quote non-elementary)))
(define rB (ar-integrate-1oversqrt (rat-zero) (rat-make (list 0 0 3) (poly-mul (list 2) p3)) p3))
(display "  INT (3x^2/2)/sqrt(x^3+1) -> ") (display (car rB)) (newline)
(chk "INT (3x^2/2)/sqrt(x^3+1) = sqrt(x^3+1), elementary and certified" (equal? (car rB) (quote elementary)))

(newline)
(display "residue classification of the differential f = u + v y:") (newline)
; v y with a simple non-branch pole: residues cancel in a conjugate pair -> second-kind (reducible)
(define c3 (ar-classify (rat-zero) (rat-make (list 1) (poly-mul (list -2 1) p3)) p3))
(display "  (1/((x-2)*sqrt(p))) y-part -> ") (display (car c3)) (display " (conjugate residues cancel)") (newline)
(chk "simple non-branch pole of the v y part is second-kind" (equal? (car c3) (quote second-kind)))
; u-part simple pole: genuine nonzero residue -> third-kind, honestly not elementary
(define cu (ar-classify (rat-make (list 1) (list -2 1)) (rat-zero) p3))
(display "  u = 1/(x-2) on the curve -> ") (display (car cu)) (display " (residue 2*res != 0)") (newline)
(chk "u-part simple pole is third-kind (has-residues), honestly NOT claimed elementary" (equal? (car cu) (quote has-residues)))

(newline)
(display "soundness (never a guessed answer):") (newline)
(define ch (ar-classify (rat-zero) (rat-make (list 1) (poly-mul (poly-mul (list -2 1) (list -2 1)) p3)) p3))
(display "  1/((x-2)^2 sqrt(p)) [higher-order pole] -> ") (display (car ch)) (newline)
(chk "higher-order non-branch pole -> not-handled (deferred to the Hermite rung)" (equal? (car ch) (quote not-handled)))

(newline)
(display "RUNG 1 reached: residues of an algebraic differential computed; second-kind decided + certified; third-kind honestly reported.") (newline)
