; The TORSION DECISION on the genus-2 Jacobian: deciding whether a divisor class is torsion (and its order), the
; genus-2 analogue of the elliptic order test that decides third-kind elementarity (docs/TRAGER_ROADMAP.md -- the
; full third-kind construction beyond genus 1).
;
; On a genus-1 curve, INT dx/((x-s) sqrt(p)) is elementary exactly when the pole lifts to a torsion point.  The
; genus-2 statement replaces the elliptic group by the Jacobian (hyperjac): the third-kind class [P] - [iota P] is
; elementary iff it is torsion, n*[D] = 0.  In Mumford terms that class is the divisor of P, so its order under
; the Jacobian group law is the torsion order.  A bounded multiple search confirms torsion (with order) or reports
; an HONEST bounded miss -- never a false claim of non-elementarity.
(import "cas/hyperjactor.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define f (list 1 0 0 0 0 1))   ; y^2 = x^5 + 1, a genus-2 curve

(display "Torsion on the Jacobian of y^2 = x^5 + 1 decides genus-2 third-kind elementarity.") (newline) (newline)

(display "basic orders:") (newline)
(must "the identity has order 1" (equal? (hjt-order f (hj-identity) 20) (list (quote torsion) 1)))
(define Q (hj-point -1 0))       ; the Weierstrass point (-1, 0)
(must "a Weierstrass point (-1, 0) is 2-torsion" (equal? (hjt-order f Q 20) (list (quote torsion) 2)))

(display "a genuine higher-order torsion class: the point (0, 1) on y^2 = x^5 + 1:") (newline)
(define ordP (hjt-order f (hj-point 0 1) 30))
(display "  order of (0, 1): ") (display ordP) (newline)
(must "(0, 1) generates a torsion class of order 5" (equal? ordP (list (quote torsion) 5)))

(display "the third-kind elementarity decision (the integral INT dx/((x-s) y)):") (newline)
(must "pole at the Weierstrass point x = -1 is elementary (torsion 2)" (equal? (hjt-third-kind-decision f -1 0 20) (list (quote elementary) (quote torsion) 2)))
(must "pole lifting to (0, 1) is elementary (torsion 5)" (equal? (hjt-third-kind-decision f 0 1 30) (list (quote elementary) (quote torsion) 5)))

(display "soundness: a search bound too small returns an HONEST bounded miss, not a non-elementary claim:") (newline)
(must "order of (0, 1) with bound 2 is reported as no-torsion-up-to 2" (equal? (hjt-order f (hj-point 0 1) 2) (list (quote no-torsion-up-to) 2)))
(must "the decision never asserts non-elementarity, only bounded-undecided" (equal? (car (hjt-third-kind-decision f 0 1 2)) (quote undecided-up-to)))

(display "a cross-check on a different genus-2 curve y^2 = x^5 - x:") (newline)
(define f2 (list 0 -1 0 0 0 1))
(must "(1, 0) is 2-torsion on y^2 = x^5 - x" (equal? (hjt-order f2 (hj-point 1 0) 20) (list (quote torsion) 2)))

(newline)
(display "The genus-2 third-kind elementarity decision is now in place: the Jacobian torsion order, computed by") (newline)
(display "Cantor's group law and reported with honest bounded negatives, decides when the integral is elementary --") (newline)
(display "moving the third-kind construction past genus 1.  Constructing the explicit algebraic logarithm in the") (newline)
(display "torsion case (the genus-2 Mumford analogue of the elliptic Miller function) is the next step.") (newline)
