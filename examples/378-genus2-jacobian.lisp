; The JACOBIAN GROUP LAW of a genus-2 hyperelliptic curve y^2 = f(x) (deg f = 5), by Mumford representation and
; Cantor's algorithm -- the higher-genus analogue of the elliptic chord-tangent law that the third-kind
; construction needs to move past genus 1 (docs/TRAGER_ROADMAP.md -- the full third-kind / general algebraic Risch
; frontier: divisor arithmetic on the Jacobian).
;
; A reduced divisor of degree <= 2 is a Mumford pair [u, v] with u monic, deg v < deg u <= 2, and u | (v^2 - f).
; The identity is [1, 0]; a point (a, b) is [x - a, b]; negation is [u, -v].  Cantor's algorithm composes and
; reduces to add divisor classes.  Every result is checked against the curve condition.
(import "cas/hyperjac.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define f (list 1 0 0 0 0 1))   ; y^2 = x^5 + 1, a genus-2 curve

(display "The Jacobian of y^2 = x^5 + 1: divisor arithmetic by Mumford representation + Cantor's algorithm.") (newline) (newline)

(display "the extended gcd over Q[x] gives correct Bezout cofactors:") (newline)
(define eg (hj-egcd (list -1 0 1) (list -1 1)))   ; gcd(x^2-1, x-1)
(must "gcd(x^2 - 1, x - 1) = x - 1" (equal? (poly-norm (car eg)) (list -1 1)))
(must "s(x^2-1) + t(x-1) = x - 1" (equal? (poly-norm (poly-add (poly-mul (car (cdr eg)) (list -1 0 1)) (poly-mul (car (cdr (cdr eg))) (list -1 1)))) (list -1 1)))

(display "divisors satisfy the Mumford curve condition u | (v^2 - f):") (newline)
(define P (hj-point 0 1))     ; the point (0, 1)
(define Q (hj-point -1 0))    ; the Weierstrass point (-1, 0)
(must "the identity [1, 0] is valid" (hj-valid? f (hj-identity)))
(must "P = (0, 1) is a valid divisor" (hj-valid? f P))
(must "Q = (-1, 0) is a valid divisor" (hj-valid? f Q))

(display "the group law: identity, addition, negation:") (newline)
(must "P + identity = P" (hj-equal? (hj-add f P (hj-identity)) P))
(display "  P + Q = ") (display (hj-add f P Q)) (newline)
(must "P + Q = [x^2 + x, x + 1], a degree-2 divisor" (hj-equal? (hj-add f P Q) (list (list 0 1 1) (list 1 1))))
(must "P + Q satisfies the curve condition" (hj-valid? f (hj-add f P Q)))
(must "P + (-P) = identity" (hj-equal? (hj-add f P (hj-neg P)) (hj-identity)))

(display "torsion: a Weierstrass point (y = 0) is 2-torsion on the Jacobian:") (newline)
(must "2 * Q = identity for Q = (-1, 0)" (hj-equal? (hj-double f Q) (hj-identity)))
(must "3 * Q = Q (odd multiples return to Q)" (hj-equal? (hj-mul f 3 Q) Q))

(newline)
(display "The Jacobian group law of a genus-2 curve is now exact -- Cantor composition and reduction over Q[x],") (newline)
(display "certified by the Mumford invariant -- giving the divisor arithmetic and torsion test that the full") (newline)
(display "third-kind construction needs in genus 2.  Building the principal-divisor function (the genus-2 Miller") (newline)
(display "/ Mumford analogue of the elliptic logarithm) on top of this group law is the next step.") (newline)
