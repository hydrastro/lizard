; RUNG 3b of the Trager-Bronstein climb (docs/TRAGER_ROADMAP.md): the GENUS-1 (elliptic) THIRD-KIND decision
; by a TORSION TEST on the elliptic curve's group -- the real Trager divisor condition, the first place where
; deciding elementarity requires genuine arithmetic geometry.
;
; On y^2 = p with p a squarefree cubic (genus 1), a simple pole at x = s lifts to the point P = (s, rho) on the
; curve (rho^2 = p(s)).  By the Abel-Jacobi theorem the residue divisor is principal -- so the third-kind
; logarithm exists and the integral is ELEMENTARY -- exactly when P is a TORSION point of the curve; otherwise
; the integral is provably NON-ELEMENTARY (the canonical elliptic obstruction).  This is the classical
; Trager/Davenport criterion (cf. Combot, arXiv:2303.14013: for y^2 = z(z-1)(z-kappa) the third-kind integral
; is elementary iff (u, sqrt(p(u))) is a torsion point).
;
; The test is decidable and terminates by Nagell-Lutz + Mazur: on an integral model a torsion point has integer
; coordinates and order at most 12, so computing P, 2P, ... either returns to the point at infinity O (torsion)
; or acquires a non-integer coordinate / exceeds the bound (non-torsion).  The group law is over Q with exact
; rational arithmetic.
(import "cas/elltorsion.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Genus-1 third-kind decision: INT dx/((x-s) sqrt(p)) elementary <=> (s, sqrt(p(s))) is torsion on y^2=p.") (newline) (newline)

(display "the elliptic group law over Q (sanity):") (newline)
(display "  (0,1) on y^2 = x^3+1 has order ") (display (elt-order (cons 0 1) 0 0)) (newline)
(chk "(0,1) is torsion of order 3" (= (elt-order (cons 0 1) 0 0) 3))
(display "  (2,3) on y^2 = x^3+1 has order ") (display (elt-order (cons 2 3) 0 0)) (newline)
(chk "(2,3) is torsion of order 6" (= (elt-order (cons 2 3) 0 0) 6))
(display "  (3,5) on y^2 = x^3-2 has order ") (display (elt-order (cons 3 5) 0 0)) (newline)
(chk "(3,5) is infinite order (non-torsion)" (equal? (elt-order (cons 3 5) 0 0) (quote infinite)))

(newline)
(display "the decision:") (newline)
(define dE (elt-decide (rat-from-poly (list 1 0 0 1)) 0))
(display "  INT dx/(x sqrt(x^3+1))  [pole lifts to (0,1), torsion] -> ") (display dE) (newline)
(chk "INT dx/(x sqrt(x^3+1)) ELEMENTARY (torsion pole, elliptic logarithm of order 3)" (equal? (car dE) (quote elementary)))

(define dN (elt-decide (rat-from-poly (list -2 0 0 1)) 3))
(display "  INT dx/((x-3) sqrt(x^3-2))  [pole lifts to (3,5), infinite order] -> ") (display dN) (newline)
(chk "INT dx/((x-3) sqrt(x^3-2)) NON-ELEMENTARY (infinite-order pole, the elliptic obstruction)" (equal? (car dN) (quote non-elementary)))

(define dE2 (elt-decide (rat-from-poly (list 4 0 0 1)) 0))
(display "  INT dx/(x sqrt(x^3+4))  [pole lifts to (0,2), torsion] -> ") (display dE2) (newline)
(chk "INT dx/(x sqrt(x^3+4)) ELEMENTARY (torsion pole)" (equal? (car dE2) (quote elementary)))

(newline)
(display "soundness (no guessed answers):") (newline)
(define dX (elt-decide (rat-from-poly (list 1 0 0 1)) 1))
(display "  INT dx/((x-1) sqrt(x^3+1))  [p(1)=2 not a perfect square] -> ") (display dX) (newline)
(chk "non-rational lift honestly reported needs-extension" (equal? (car dX) (quote needs-extension)))

(newline)
(display "RUNG 3b reached: the elliptic torsion test decides genus-1 third-kind elementarity over Q, sound, with the canonical elliptic non-elementarity proven.") (newline)
