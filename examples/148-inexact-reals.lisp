; 148-inexact-reals.lisp — the numeric tower now has inexact (floating-point)
; reals alongside exact integers and exact rationals, with R7RS-style
; exact/inexact contagion: any inexact operand makes the result inexact, while
; all-exact computations stay exact.
;
; Self-checking: each `must` raises on failure (non-zero exit).

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'real-check-failed)))

(define (close? a b) (< (abs (- a b)) 0.0000001))

(display "inexact reals: tower, contagion, predicates, math") (newline)
(newline)
(display "Exact stays exact; inexact is contagious:") (newline)

(must "1 + 2 + 3 is exact"        (exact? (+ 1 2 3)))
(must "1/2 + 1/3 is exact"        (exact? (+ 1/2 1/3)))
(must "1 + 2.5 is inexact"        (inexact? (+ 1 2.5)))
(must "1 + 2.5 = 3.5"             (= (+ 1 2.5) 3.5))
(must "2 * 0.5 = 1.0"             (= (* 2 0.5) 1.0))
(must "exact / stays rational"    (equal? (/ 1 4) 1/4))
(must "inexact / contagion"       (= (/ 1.0 4) 0.25))
(must "rational + real = real"    (and (= (+ 1/2 0.5) 1.0) (inexact? (+ 1/2 0.5))))

(newline)
(display "Numeric = (with contagion) vs structural equal?:") (newline)

(must "(= 1 1.0) numerically"     (= 1 1.0))
(must "(= 1/2 0.5) numerically"   (= 1/2 0.5))
(must "(equal? 1 1.0) is #f"      (not (equal? 1 1.0)))
(must "ordering across types"     (and (< 1 1.5 2) (> 2.5 1/2)))

(newline)
(display "Predicates and conversions:") (newline)

(must "integer? 2.0"              (integer? 2.0))
(must "not integer? 2.5"          (not (integer? 2.5)))
(must "real? 3"                   (real? 3))
(must "rational? 2.0 (finite)"    (rational? 2.0))
(must "exact-integer? 3"          (exact-integer? 3))
(must "not exact-integer? 3.0"    (not (exact-integer? 3.0)))
(must "exact->inexact 1/2 = 0.5"  (= (exact->inexact 1/2) 0.5))
(must "inexact->exact 0.5 = 1/2"  (equal? (inexact->exact 0.5) 1/2))
(must "inexact->exact 0.25 = 1/4" (equal? (inexact->exact 0.25) 1/4))

(newline)
(display "Math: sqrt keeps perfect squares exact; transcendentals:") (newline)

(must "sqrt 16 = 4 (exact)"       (and (= (sqrt 16) 4) (exact? (sqrt 16))))
(must "sqrt 2 is inexact"         (inexact? (sqrt 2)))
(must "(sqrt 2)^2 ~ 2"            (close? (* (sqrt 2) (sqrt 2)) 2))
(must "exp 0 ~ 1"                 (close? (exp 0) 1))
(must "exp/log inverse"           (close? (log (exp 2)) 2))
(must "sin^2 + cos^2 ~ 1"         (close? (+ (* (sin 1) (sin 1)) (* (cos 1) (cos 1))) 1))
(must "floor 7/2 = 3 (exact)"     (equal? (floor 7/2) 3))
(must "ceiling 7/2 = 4 (exact)"   (equal? (ceiling 7/2) 4))
(must "round 3.7 = 4.0"           (= (round 3.7) 4.0))

(newline)
(display "Tie-in to verified integration (FTC, numerically):") (newline)
; cas/integral-cert certified  ∫cos dx = sin  (proof: Der sin cos).  Evaluate
; the definite integral numerically with the antiderivative sin and the new
; transcendental functions:  ∫_0^{π/2} cos t dt = sin(π/2) - sin(0) = 1.
(define pi (* 4 (atan 1)))
(must "pi ~ 3.14159"              (close? pi 3.14159265))
(must "∫_0^{π/2} cos = 1"         (close? (- (sin (/ pi 2)) (sin 0)) 1))
(must "∫_0^π cos = 0"             (close? (- (sin pi) (sin 0)) 0))

(newline)
(display "inexact-reals: all checks passed") (newline)
