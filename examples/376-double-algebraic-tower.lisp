; A TOWER OF TWO ALGEBRAIC EXTENSIONS -- the "two stacked monomials" rung of the algebraic frontier: arithmetic and
; the certified derivation in Q(x)[y][z]/(z^2 - y, y^2 - x), the field of x^(1/4) (docs/TRAGER_ROADMAP.md, RUNG 5 --
; deeper stacked towers past the single-extension restriction).
;
; The inner field Q(x)[y]/(y^2 - x) (y = sqrt x) is handled by algfunc; this stacks a second radical z^2 = y, so
; z = x^(1/4) and elements are a + b z with a, b in the inner field.  The derivation reduces to the trusted inner
; one plus a single scalar: from z^2 = y, z'/z = y'/(2y) = 1/(4x), so D(a + b z) = a' + (b' + b/(4x)) z.  Soundness
; is inherited from algfunc's certified derivation; every answer here is checked by differentiating in the tower.
(import "cas/algtower2.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "A tower of two radicals z^2 = y, y^2 = x (so z = x^(1/4)): arithmetic and certified derivation.") (newline) (newline)

(display "the second generator z = x^(1/4) has the scalar logarithmic derivative z'/z = 1/(4x):") (newline)
(define Dz (t2-deriv (t2-z)))
(must "D(z) has no inner-field part (the a-coordinate is 0)" (af-equal? (t2-a Dz) (af-zero)))
(must "D(z) = (1/(4x)) z, matching d/dx x^(1/4) = (1/4) x^(1/4)/x" (rat-equal? (af-u (t2-b Dz)) (rat-make (list 1) (list 0 4))))

(display "the tower reduces z^2 -> y and z^4 -> x correctly:") (newline)
(must "z^2 = y" (t2-equal? (t2-mul (t2-z) (t2-z)) (t2-y)))
(must "z^4 = x" (t2-equal? (t2-mul (t2-mul (t2-z) (t2-z)) (t2-mul (t2-z) (t2-z))) (t2-from-rat (rat-from-poly (list 0 1)))))
(must "(1 + z)(1 - z) = 1 - y" (t2-equal? (t2-mul (t2-add (t2-one) (t2-z)) (t2-sub (t2-one) (t2-z))) (t2-sub (t2-one) (t2-y))))

(display "the inner derivation y' = y/(2x) is inherited from the trusted single-extension layer:") (newline)
(must "D(y) = (1/(2x)) y" (rat-equal? (af-v (t2-a (t2-deriv (t2-y)))) (rat-make (list 1) (list 0 2))))

(display "the integration payoff: INT (5/4) x^(1/4) dx = x^(5/4), i.e. INT (5/4) z dx = x z, certified:") (newline)
(define xz (t2-make (af-zero) (af-from-rat (rat-from-poly (list 0 1)))))      ; x z
(define five-fourths-z (t2-make (af-zero) (af-from-rat (rat-from-poly (list (/ 5 4))))))
(display "  D(x z) = ") (display (t2-deriv xz)) (newline)
(must "D(x z) = (5/4) z" (t2-equal? (t2-deriv xz) five-fourths-z))
(must "the differentiation certificate confirms INT (5/4) z dx = x z" (t2-certify xz five-fourths-z))

(newline)
(display "Integration now operates in a tower of TWO stacked algebraic extensions (x^(1/4)), the derivation reducing") (newline)
(display "soundly to the trusted single-extension layer plus one scalar logarithmic-derivative term, every answer") (newline)
(display "certified by differentiation in the tower.  Deeper towers with several independent radicals, and the full") (newline)
(display "Trager-Bronstein normalization at each singular place, remain the open summit.") (newline)
