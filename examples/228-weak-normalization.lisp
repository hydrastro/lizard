; 228-weak-normalization.lisp -- weak normalization, completing the base-case Risch differential
; equation y' + f y = g over Q(x) for an ARBITRARY rational coefficient f.
;
; rderat.lisp needs f weakly normalized (no simple pole of f has a positive-integer residue).
; WeakNormalizer removes the obstruction: at a simple pole of f with positive-integer residue n a
; solution may have a pole of order n, so we build q = prod (x-a)^n over those poles and solve
; for z = q y, whose coefficient f - q'/q IS weakly normalized.  The residues are the roots of
; R(y) = res_x(a - y d', d1) with d1 the multiplicity-one part of denom(f); no factorization.
; This is essential exactly when exp(-INT f) is not rational, so the forced pole cannot simply be
; shifted away.  Every solution returned is differentiation-certified.  `must` raises on failure.

(import "cas/weaknorm.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'wn-check-failed)))

(display "Weak normalization and the full base-case Risch DE over Q(x)") (newline) (newline)

(display "1. computing q from the positive-integer residues of f") (newline)
(display "    f = (5/2 x - 6)/(x^2 - 3x): residue 2 at x=0, residue 1/2 at x=3") (newline)
(display "    q = ") (display (weak-normalizer (list -6 5/2) (list 0 -3 1))) (display "  (= x^2, from the integer residue 2)") (newline)
(must "q = x^2"                                       (equal? (weak-normalizer (list -6 5/2) (list 0 -3 1)) (list 0 0 1)))
(must "q = 1 when the only residue is non-integer"    (poly-const? (weak-normalizer (list 1) (list -6 2))))
(must "q = 1 when f = u' (all residues zero)"         (poly-const? (weak-normalizer (list -1) (list 0 0 1))))
(newline)

(display "2. the case that REQUIRES weak normalization") (newline)
(display "    f = (5/2 x - 6)/(x^2 - 3x),  g = 1/(2 x^2 (x-3));  unique rational solution y = 1/x^2") (newline)
(define fa (list -6 5/2)) (define fd (list 0 -3 1))
(define ga (list 1)) (define gd (poly-scale 2 (poly-mul (list 0 0 1) (list -3 1))))
(must "the weakly-normalized solver alone cannot see it (returns none)" (equal? (rdr-solve fa fd ga gd) 'none))
(define y (rde-general fa fd ga gd))
(display "    rde-general finds y = ") (display y) (newline)
(must "rde-general recovers y = 1/x^2"   (equal? y (cons (list 1) (list 0 0 1))))
(must "and certifies it by differentiation" (rde-general-verify fa fd ga gd y))
(newline)

(display "3. round trips: choose y and f (with an integer-residue pole), set g = y' + f y") (newline)
(define f2 (rde-radd (rde-rmake (list 3) (list 0 1)) (rde-rmake (list 1) (list -4 2))))   ; 3/x + 1/(2(x-2))
(define y2 (rde-rmake (list 1) (list 0 1)))                                                ; y = 1/x
(define g2 (rde-radd (rde-rderiv y2) (rde-rmul f2 y2)))
(must "f = 3/x + 1/(2(x-2)), y = 1/x  recovered and certified"
      (rde-general-verify (car f2) (cdr f2) (car g2) (cdr g2) (rde-general (car f2) (cdr f2) (car g2) (cdr g2))))
(define f3 (rde-rmake (list 0 2) (list 0 0 1)))                                            ; 2/x (residue 2)
(define y3 (rde-radd (rde-rmake (list 1) (list 0 0 1)) (rde-rmake (list 0 1) (list 1))))   ; 1/x^2 + x
(define g3 (rde-radd (rde-rderiv y3) (rde-rmul f3 y3)))
(must "f = 2/x, y = 1/x^2 + x  recovered and certified"
      (rde-general-verify (car f3) (cdr f3) (car g3) (cdr g3) (rde-general (car f3) (cdr f3) (car g3) (cdr g3))))
(newline)

(display "all weak-normalization checks passed.") (newline)
