; 206-elliptic-curves.lisp -- elliptic curves over a prime field F_p.
;
; The points of y^2 = x^3 + a x + b over F_p (with 4a^3 + 27b^2 /= 0) plus a point at
; infinity form an abelian group under the chord-and-tangent law.  This example builds the
; group, multiplies points, counts the group, and checks the deep structural laws: every
; sum stays on the curve (closure), the law is associative, O and -P are identity and
; inverse, the count obeys the Hasse bound, and every point's order divides #E (Lagrange).
; `must` raises on failure.

(import "cas/ec.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'ec-check-failed)))

(display "Elliptic curves over F_p") (newline) (newline)

(display "1. the curve y^2 = x^3 + 2x + 2 over F_17") (newline)
(display "    #E(F_17) = ") (display (ec-count 2 2 17)) (newline)
(define P (cons 5 1))
(display "    P = ") (display (pt->string P)) (display ",  2P = ") (display (pt->string (ec-double P 2 17))) (display ",  3P = ") (display (pt->string (ec-mul 3 P 2 17))) (newline)
(must "the curve is nonsingular"   (nonsingular? 2 2 17))
(must "#E(F_17) = 19"              (= (ec-count 2 2 17) 19))
(must "P = (5,1) is on the curve"  (on-curve? P 2 2 17))
(must "2P and 3P are on the curve" (and (on-curve? (ec-double P 2 17) 2 2 17) (on-curve? (ec-mul 3 P 2 17) 2 2 17)))
(must "P has order 19 (a generator)" (= (ec-order P 2 17) 19))
(must "19 * P = O"                (equal? (ec-mul 19 P 2 17) 'O))
(must "associativity P+Q+R holds for a sample" (assoc-triple (cons 5 1) (cons 6 3) (cons 10 6) 2 17))
(newline)

(display "2. the group laws verified over the WHOLE group") (newline)
(must "closure: every P+Q is on the curve"  (ec-closure-ok? 2 2 17))
(must "associativity: holds for all triples" (ec-assoc-ok? 2 2 17))
(must "inverse: P + (-P) = O for every P"    (ec-inverse-ok? 2 2 17))
(must "Hasse bound |#E-(p+1)| <= 2 sqrt p"   (hasse-ok? 2 2 17))
(must "Lagrange: every point order divides #E" (ec-lagrange-ok? 2 2 17))
(newline)

(display "3. more curves obey the same laws") (newline)
(display "    #E for y^2=x^3+x+1 over F_5, y^2=x^3+x+6 over F_11, y^2=x^3+2x+2 over F_23: ")
(display (list (ec-count 1 1 5) (ec-count 1 6 11) (ec-count 2 2 23))) (newline)
(must "F_5 curve: closure, Hasse, Lagrange"  (and (ec-closure-ok? 1 1 5) (hasse-ok? 1 1 5) (ec-lagrange-ok? 1 1 5)))
(must "F_11 curve: closure, Hasse, Lagrange" (and (ec-closure-ok? 1 6 11) (hasse-ok? 1 6 11) (ec-lagrange-ok? 1 6 11)))
(must "F_23 curve: closure, Hasse, Lagrange" (and (ec-closure-ok? 2 2 23) (hasse-ok? 2 2 23) (ec-lagrange-ok? 2 2 23)))
(newline)

(display "all elliptic-curve checks passed.") (newline)
