; 223-rothstein-trager.lisp -- the logarithmic part of integration (Rothstein-Trager).
;
; For a proper rational a/b with b squarefree, INT a/b dx = sum_i c_i log(v_i).  Rothstein and
; Trager find it without factoring b: the constants c_i are the roots of the resultant
; R(y) = res_x(a - y b', b) (the residues of a/b), and v_c = gcd(a - c b', b).  Here the answer
; is assembled over the rational roots of R and certified by differentiation -- the derivative
; sum c_i v_i'/v_i is checked to equal a/b exactly.  When every residue is rational the
; logarithmic part is complete; an irreducible denominator (x^2+1) has algebraic residues and is
; reported as such.  `must` raises on failure.

(import "cas/rothstein.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'rt-check-failed)))

(display "Rothstein-Trager logarithmic integration") (newline) (newline)

(display "1. INT 1/(x^2 - 1) dx") (newline)
(define a1 (list 1)) (define b1 (list -1 0 1))
(display "    resultant R(y) = ") (display (rt-resultant a1 b1)) (display "  (roots are the residues +-1/2)") (newline)
(display "    log part (coeff . arg pairs) = ") (display (rt-log-part a1 b1)) (newline)
(must "b is squarefree"                        (ros-squarefree? b1))
(must "the result is certified by differentiation" (rt-verify a1 b1))
(must "all residues rational, so the log part is complete" (rt-complete? a1 b1))
(newline)

(display "2. INT 1/(x^3 - x) dx and INT (3x+1)/((x-1)(x-2)) dx") (newline)
(define b2 (list 0 -1 0 1))
(define b3 (poly-mul (list -1 1) (list -2 1)))
(must "1/(x^3-x) is certified"                 (rt-verify (list 1) b2))
(must "1/(x^3-x) is complete"                  (rt-complete? (list 1) b2))
(must "(3x+1)/((x-1)(x-2)) is certified"       (rt-verify (list 1 3) b3))
(display "    (3x+1)/((x-1)(x-2)) log part = ") (display (rt-log-part (list 1 3) b3)) (newline)
(newline)

(display "3. a denominator irreducible over Q has algebraic residues") (newline)
(define bq (list 1 0 1))
(display "    for x^2 + 1: R(y) = ") (display (rt-resultant (list 1) bq)) (display ", rational roots = ") (display (ros-rational-roots (rt-resultant (list 1) bq))) (newline)
(must "x^2 + 1 has no rational residues (incomplete over Q)" (not (rt-complete? (list 1) bq)))
(newline)

(display "all Rothstein-Trager checks passed.") (newline)
