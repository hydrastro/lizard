; 167-groebner.lisp — multivariate polynomials over Q and Groebner bases.
;
; Monomials are exponent vectors; a polynomial is a lex-sorted list of
; (coeff . monomial) terms.  Buchberger's algorithm computes a Groebner basis G of
; the ideal <F>; because G is built from F by ideal operations, <G> = <F>, and G is
; certified to be a *Groebner* basis by Buchberger's criterion (every S-polynomial
; reduces to 0 modulo G).  Normal form then decides ideal membership and a lex basis
; eliminates variables, so a polynomial system is "solved" by reading off the
; triangular basis.  `must` raises on failure.

(import "cas/groebner.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'gb-check-failed)))
(define vars (list "x" "y"))
; term constructors over (x,y)
(define (X a) (cons 1 (list a 0)))
(define x-y (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))
(define x+y (list (cons 1 (list 1 0)) (cons 1 (list 0 1))))

(display "Multivariate polynomials and Groebner bases") (newline) (newline)

(display "1. arithmetic, normal form, ideal membership") (newline)
(must "(x+y)(x-y) = x^2 - y^2" (equal? (mpoly-mul x+y x-y) (list (cons 1 (list 2 0)) (cons -1 (list 0 2)))))
(must "x^2 - y^2 reduces to 0 mod (x-y)" (in-ideal? (mpoly-mul x+y x-y) (list x-y)))
(must "x^2 + y^2 is NOT in <x-y>" (not (in-ideal? (list (cons 1 (list 2 0)) (cons 1 (list 0 2))) (list x-y))))
(newline)

(display "2. solve x^2+y^2=1, x=y  (lex elimination)") (newline)
(define circle-line (reduced-groebner (list (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -1 (list 0 0))) x-y)))
(for-each (lambda (g) (display "    ") (display (mpoly->str g vars)) (newline)) circle-line)
(must "basis is { x - y , y^2 - 1/2 }"
      (equal? circle-line (list x-y (list (cons 1 (list 0 2)) (cons (/ -1 2) (list 0 0))))))
(must "Buchberger criterion holds" (groebner-ok? circle-line))
(newline)

(display "3. solve xy=1, x=y  (hyperbola meets line)") (newline)
(define hyp-line (reduced-groebner (list (list (cons 1 (list 1 1)) (cons -1 (list 0 0))) x-y)))
(for-each (lambda (g) (display "    ") (display (mpoly->str g vars)) (newline)) hyp-line)
(must "basis is { x - y , y^2 - 1 }  (so y = +/-1, x = y)"
      (equal? hyp-line (list x-y (list (cons 1 (list 0 2)) (cons -1 (list 0 0))))))
(newline)

(display "4. two circles x^2+y^2=4, (x-1)^2+y^2=1  (tangent)") (newline)
(define two-circ (reduced-groebner (list (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -4 (list 0 0)))
                                         (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -2 (list 1 0))))))
(for-each (lambda (g) (display "    ") (display (mpoly->str g vars)) (newline)) two-circ)
(must "basis is { x - 2 , y^2 }  (tangent at x=2, y=0)"
      (equal? two-circ (list (list (cons 1 (list 1 0)) (cons -2 (list 0 0))) (list (cons 1 (list 0 2))))))
(newline)

(display "5. inconsistent system x=1, x=2") (newline)
(define inconsistent (reduced-groebner (list (list (cons 1 (list 1 0)) (cons -1 (list 0 0))) (list (cons 1 (list 1 0)) (cons -2 (list 0 0))))))
(must "Groebner basis is { 1 }  (1 in ideal => no solutions)" (equal? inconsistent (list (list (cons 1 (list 0 0))))))
(must "every polynomial lies in the unit ideal" (in-ideal? (mpoly-mul x+y x-y) inconsistent))
(newline)

(display "all Groebner checks passed.") (newline)
