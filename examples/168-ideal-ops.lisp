; 168-ideal-ops.lisp — ideal operations and polynomial system solving on top of
; Groebner bases, composed with the univariate exact-root solver.
;
;   * elimination ideal: from a lex Groebner basis, the generators in the later
;     variables only (Elimination Theorem) -- this projects a variety.
;   * ideal sum and ideal intersection (the latter via the fresh-variable t-trick).
;   * zero-dimensional solving: a lex basis is triangular, so its generator in the
;     last variable is univariate; handed to solve-poly it yields exact roots,
;     joining the multivariate and univariate machinery.
; Results are checked by ideal membership and by verifying the exact roots.
; `must` raises on failure.

(import "cas/idealops.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'ideal-check-failed)))
(define vars (list "x" "y"))
(define x-y (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))
(define circle (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -1 (list 0 0))))
(define y2-half (list (cons 1 (list 0 2)) (cons (/ -1 2) (list 0 0))))

(display "Ideal operations and polynomial system solving") (newline) (newline)

(display "1. elimination ideal (project a variety)") (newline)
(define Gcl (reduced-groebner (list circle x-y)))
(must "eliminating x from {x^2+y^2-1, x-y} gives {y^2-1/2}" (equal? (elim-ideal Gcl 1) (list y2-half)))
(must "that polynomial lies in the ideal" (in-ideal? y2-half Gcl))
(newline)

(display "2. ideal sum and intersection") (newline)
(define Ix (list (list (cons 1 (list 1 0)))))
(define Iy (list (list (cons 1 (list 0 1)))))
(must "<x> ∩ <y> = <xy>"        (equal? (ideal-intersect Ix Iy 2) (list (list (cons 1 (list 1 1))))))
(must "<x^2> ∩ <x> = <x^2>"     (equal? (ideal-intersect (list (list (cons 1 (list 2 0)))) Ix 2) (list (list (cons 1 (list 2 0))))))
(must "<x-y> + <x^2+y^2-1> = the circle-line basis" (equal? (ideal-sum (list x-y) (list circle)) Gcl))
(newline)

(display "3. solve a system exactly: x^2+y^2=1, x=y") (newline)
(define sols-cl (solve-last Gcl 2))
(display "    y satisfies: ") (display (mpoly->str (last-generator Gcl 2) vars)) (newline)
(display "    y = ") (display (solve-last->string Gcl 2)) (newline)
(must "two solutions for y"          (= (length sols-cl) 2))
(must "each root satisfies y^2-1/2"  (solutions-verify (mv->uni (last-generator Gcl 2) 2) sols-cl))
(newline)

(display "4. solve a system exactly: xy=1, x=y") (newline)
(define Ghl (reduced-groebner (list (list (cons 1 (list 1 1)) (cons -1 (list 0 0))) x-y)))
(define sols-hl (solve-last Ghl 2))
(display "    y = ") (display (solve-last->string Ghl 2)) (newline)
(must "roots are y = 1 and y = -1" (solutions-verify (mv->uni (last-generator Ghl 2) 2) sols-hl))
(newline)

(display "all ideal-operation checks passed.") (newline)
