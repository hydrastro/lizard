; 225-rational-integration-complete.lisp -- complete rational-function integration, the Risch way.
;
; Composing the two halves of the rational case of the Risch algorithm: Hermite reduction takes
; the rational part, Rothstein-Trager integrates the squarefree remainder as a sum of
; logarithms, and neither step factors the denominator into irreducibles.  The answer
;   INT A/D dx = ratnum/ratden + sum_i c_i log(v_i)
; is certified by differentiating it back to A/D.  It is complete exactly when the denominator
; splits over Q (rational residues); an irreducible quadratic factor contributes an arctangent
; whose residues are algebraic, which is reported honestly while the Hermite rational part stays
; exact.  `must` raises on failure.

(import "cas/rischrat.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'ri-check-failed)))
(define (report A D name)
  (let ((r (rat-integrate A D)))
    (display "    INT ") (display name) (display " dx  =  [") (display (car r)) (display "]/[") (display (ri-cadr r))
    (display "]  +  ") (display (ri-caddr r)) (newline)))

(display "complete rational integration (Hermite + Rothstein-Trager)") (newline) (newline)

(display "1. INT 1/(x^2 (x-1)) dx -- a rational part and two logs") (newline)
(define A1 (list 1)) (define D1 (poly-mul (list 0 0 1) (list -1 1)))
(report A1 D1 "1/(x^2(x-1))")
(must "complete over Q"                       (rat-integrate-complete? A1 D1))
(must "certified by differentiation"          (rat-integrate-verify A1 D1))
(newline)

(display "2. INT (2x+1)/(x^3 - x) dx -- denominator already squarefree, pure log part") (newline)
(define A2 (list 1 2)) (define D2 (list 0 -1 0 1))
(report A2 D2 "(2x+1)/(x^3-x)")
(must "complete over Q"                       (rat-integrate-complete? A2 D2))
(must "certified by differentiation"          (rat-integrate-verify A2 D2))
(newline)

(display "3. more all-rational-residue integrands, certified end to end") (newline)
(must "INT 1/((x-1)^2 (x-2)) dx certified"    (rat-integrate-verify (list 1) (poly-mul (hm-pow (list -1 1) 2) (list -2 1))))
(must "INT (x+3)/(x^2 (x^2-1)) dx certified"  (rat-integrate-verify (list 3 1) (poly-mul (list 0 0 1) (list -1 0 1))))
(newline)

(display "4. an irreducible quadratic factor -> algebraic residues (arctangent)") (newline)
(define A4 (list 2 3 5)) (define D4 (list -1 0 0 0 1))   ; (5x^2+3x+2)/(x^4-1), x^4-1 has factor x^2+1
(report A4 D4 "(5x^2+3x+2)/(x^4-1)")
(must "honestly reported as NOT complete over Q" (not (rat-integrate-complete? A4 D4)))
(must "yet the Hermite rational part is exact"   (rat-integrate-rational-ok? A4 D4))
(newline)

(display "all rational-integration checks passed.") (newline)
