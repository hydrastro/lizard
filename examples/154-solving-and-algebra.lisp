; 154-solving-and-algebra.lisp — resultants & discriminants, exact algebraic
; numbers, and equation solving over Q-bar with real-root counting.
;
; Built on the certified factorizer.  `solve-poly` factors and reads roots off
; the irreducible factors: linear -> rational, quadratic -> exact surds in
; Q(sqrt D), degree>=3 -> RootOf annotated with the number of real roots from a
; Sturm sequence.  Rational and surd roots are CERTIFIED by substituting them
; back (in Q, or in Q(sqrt D)) and checking zero, so a wrong root cannot pass.
; `must` raises on failure.

(import "cas/solve.lisp")
(import "cas/resultant.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'solve-check-failed)))

(display "resultants, algebraic numbers, equation solving") (newline) (newline)

; ============================================================
; 1. resultants and discriminants
; ============================================================
(display "1. resultants & discriminants") (newline)
(must "res(x^2-1, x-1) = 0  (shared root)"   (= (resultant (list -1 0 1) (list -1 1)) 0))
(must "res(x^2-1, x^2-4) = 9  (coprime)"     (= (resultant (list -1 0 1) (list -4 0 1)) 9))
(must "res = 0  iff  gcd is nonconstant"
      (and (= 0 (resultant (list -1 0 1) (list 2 -3 1)))          ; x^2-1, x^2-3x+2 share (x-1)
           (> (poly-deg (poly-gcd (list -1 0 1) (list 2 -3 1))) 0)
           (not (= 0 (resultant (list -1 0 1) (list -4 0 1))))    ; x^2-1, x^2-4 coprime
           (= 0 (poly-deg (poly-gcd (list -1 0 1) (list -4 0 1))))))
(must "disc(x^2-2) = 8"        (= (discriminant (list -2 0 1)) 8))
(must "disc(x^3-2) = -108"     (= (discriminant (list -2 0 0 1)) -108))
(must "disc((x-1)^2) = 0"      (= (discriminant (list 1 -2 1)) 0))
(newline)

; ============================================================
; 2. exact algebraic-number arithmetic in Q(alpha)
; ============================================================
(display "2. algebraic numbers") (newline)
(define s2min (list -2 0 1)) (define s2 (alg-gen s2min))         ; sqrt 2
(must "sqrt(2)^2 = 2"                 (alg-equal? (alg-mul s2 s2) (alg-from-q s2min 2)))
(must "(1+s2)(1-s2) = -1"
      (alg-equal? (alg-mul (alg-add (alg-one s2min) s2) (alg-sub (alg-one s2min) s2))
                  (alg-from-q s2min -1)))
(must "sqrt(2) * 1/sqrt(2) = 1"       (alg-equal? (alg-mul s2 (alg-inv s2)) (alg-one s2min)))
(define phimin (list -1 -1 1)) (define phi (alg-gen phimin))     ; golden ratio
(must "phi^2 = phi + 1"               (alg-equal? (alg-mul phi phi) (alg-add phi (alg-one phimin))))
(define c2min (list -2 0 0 1)) (define c2 (alg-gen c2min))       ; cube root of 2
(must "cbrt(2)^3 = 2"                 (alg-equal? (alg-mul (alg-mul c2 c2) c2) (alg-from-q c2min 2)))
(newline)

; ============================================================
; 3. equation solving — every rational/surd root substitutes back to 0
; ============================================================
(display "3. equation solving (roots verified by substitution)") (newline)
(define (solve-ok? p) (solutions-verify p (solve-poly p)))
(must "x^2 - 5x + 6     -> {2, 3}"              (solve-ok? (list 6 -5 1)))
(must "x^2 - 2          -> +- sqrt(2)"          (solve-ok? (list -2 0 1)))
(must "2x^2 - 3x - 2    -> {2, -1/2}"           (solve-ok? (list -2 -3 2)))
(must "x^2 + 1          -> +- i"                (solve-ok? (list 1 0 1)))
(must "x^2 + x - 1      -> golden surds"        (solve-ok? (list -1 1 1)))
(must "(x-1)^2(x+3)     -> roots w/ multiplicity" (solve-ok? (poly-mul (poly-pow (list -1 1) 2) (list 3 1))))
(must "x^3 - 2          -> RootOf + rationals"  (solve-ok? (list -2 0 0 1)))
(must "x^4 - 1          -> {1,-1,i,-i}"         (solve-ok? (list -1 0 0 0 1)))
; structural facts: number of solutions, and Sturm real-root counts
(must "x^2-5x+6 yields 2 solutions"            (= (length (solve-poly (list 6 -5 1))) 2))
(must "sqrt(2) really solves x^2-2"            (alg-root? (list -2 0 1) s2))
(newline)

; ============================================================
; 4. Sturm: exact count of REAL roots
; ============================================================
(display "4. Sturm real-root counts") (newline)
(must "x^2 - 2 has 2 real roots"               (= (count-real-roots (list -2 0 1)) 2))
(must "x^2 + 1 has 0 real roots"               (= (count-real-roots (list 1 0 1)) 0))
(must "x^3 - 2 has 1 real root"                (= (count-real-roots (list -2 0 0 1)) 1))
(must "x^4 - 10x^2 + 1 has 4 real roots"       (= (count-real-roots (list 1 0 -10 0 1)) 4))
(must "(x-1)(x-2)(x-3) has 3 real roots in (0,4]"
      (= (count-real-roots-in (poly-mul (poly-mul (list -1 1) (list -2 1)) (list -3 1)) 0 4) 3))
(newline)

(display "all solving/algebra checks passed.") (newline)
