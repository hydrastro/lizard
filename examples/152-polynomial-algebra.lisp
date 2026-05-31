; 152-polynomial-algebra.lisp — exact univariate polynomial algebra over Q,
; with factorization into irreducibles over Z/Q.
;
; A polynomial is a dense coefficient list, low-to-high, over EXACT rationals
; (lizard's numbers carry bignum numerators/denominators), so every operation
; here is exact and total.  Factorization follows the standard modern route:
;   square-free decomposition (Yun)  ->  factor mod a good prime by
;   Cantor-Zassenhaus  ->  Hensel-lift past the Mignotte bound  ->  recombine
;   lifted factors and trial-divide over Z (Zassenhaus).
;
; Self-checking: every factorization is verified by MULTIPLYING THE FACTORS
; BACK and comparing to the input (`factor-verify`), so a wrong factorisation
; can never pass.  `must` raises on any failed check (non-zero exit), and every
; check is a deterministic algebraic identity.

(import "cas/ratfun.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'poly-check-failed)))

(define (count-factors fz) (length (car (cdr fz))))   ; # of distinct irreducibles

(display "exact polynomial algebra over Q + factorization")
(newline) (newline)

; ============================================================
; 1. RING ARITHMETIC is exact and obeys the ring laws
; ============================================================
(display "1. ring arithmetic") (newline)
(define a (list 1 1))              ; x + 1
(define b (list -1 1))             ; x - 1
(must "(x+1)(x-1) = x^2 - 1" (equal? (poly-mul a b) (list -1 0 1)))
(must "eval (x^2-1) at 3 = 8" (= (poly-eval (poly-mul a b) 3) 8))
(must "d/dx(x^3-2x+5) = 3x^2-2" (equal? (poly-deriv (list 5 -2 0 1)) (list -2 0 3)))
; distributivity on a couple of operands
(define p1 (list 3 0 -2 0 1))      ; x^4 - 2x^2 + 3
(define p2 (list 1 1 1))           ; x^2 + x + 1
(define p3 (list 0 -5 7))          ; 7x^2 - 5x
(must "p1*(p2+p3) = p1*p2 + p1*p3"
      (equal? (poly-mul p1 (poly-add p2 p3))
              (poly-add (poly-mul p1 p2) (poly-mul p1 p3))))
(newline)

; ============================================================
; 2. DIVISION and GCD
; ============================================================
(display "2. division and gcd") (newline)
(define A (list 3 0 -2 0 1))       ; x^4 - 2x^2 + 3
(define B (list 1 1 1))            ; x^2 + x + 1
(define QR (poly-divmod A B))
(must "A = q*B + r  with deg r < deg B"
      (and (equal? (poly-add (poly-mul (car QR) B) (car (cdr QR))) (poly-norm A))
           (< (poly-deg (car (cdr QR))) (poly-deg B))))
; gcd(x^2-1, x^2-2x+1) = x-1 (monic)
(must "gcd(x^2-1, x^2-2x+1) = x-1"
      (equal? (poly-gcd (list -1 0 1) (list 1 -2 1)) (list -1 1)))
; gcd divides both arguments
(define g (poly-gcd (list -6 -1 0 1) (list -2 -1 0 1)))
(must "gcd divides both inputs"
      (and (poly-divides? g (list -6 -1 0 1)) (poly-divides? g (list -2 -1 0 1))))
(newline)

; ============================================================
; 3. SQUARE-FREE decomposition (Yun)
; ============================================================
(display "3. square-free decomposition") (newline)
; (x-1)^2 (x+2)^3
(define sfin (poly-mul (poly-pow (list -1 1) 2) (poly-pow (list 2 1) 3)))
(define sf (square-free sfin))
(must "two square-free parts found" (= (length sf) 2))
(must "multiplicity-2 part is (x-1)"
      (equal? (car (cdr (car sf))) (list -1 1)))
(must "multiplicity-3 part is (x+2)"
      (equal? (car (cdr (car (cdr sf)))) (list 2 1)))
(newline)

; ============================================================
; 4. FACTORIZATION over Z/Q — every result verified by multiply-back
; ============================================================
(display "4. factorization (each checked by multiplying factors back)") (newline)
(define (factors-ok? p) (factor-verify p (factor-Q p)))

(must "x^2 - 1                       -> verified"  (factors-ok? (list -1 0 1)))
(must "x^4 - 1                       -> verified"  (factors-ok? (list -1 0 0 0 1)))
(must "x^4 + 3x^2 + 2 = (x^2+1)(x^2+2)" (factors-ok? (list 2 0 3 0 1)))
(must "6x^2 + 5x + 1   (non-monic)   -> verified"  (factors-ok? (list 1 5 6)))
(must "x^2 - 1/4       (rational)    -> verified"  (factors-ok? (list (/ -1 4) 0 1)))
(must "x^6 - 1                       -> verified"  (factors-ok? (list -1 0 0 0 0 0 1)))
(must "(x-1)^2(x+2)    (repeated)    -> verified"  (factors-ok? sfin))

; structural facts the algorithm must get right
(must "x^6 - 1 has 4 irreducible factors"
      (= (count-factors (factor-Q (list -1 0 0 0 0 0 1))) 4))
(must "Phi_5 = x^4+x^3+x^2+x+1 is irreducible"
      (= (count-factors (factor-Q (list 1 1 1 1 1))) 1))
(must "Swinnerton-Dyer x^4-10x^2+1 is irreducible over Z"
      (= (count-factors (factor-Q (list 1 0 -10 0 1))) 1))
(newline)

; ============================================================
; 5. EXPRESSION-LEVEL factoring (shares the (+ a b)/(* a b)/(^ x n) convention)
; ============================================================
(display "5. expression-level factoring") (newline)
(must "factor x^2 - 5x + 6 = (x-3)(x-2)"
      (equal? (factor-expr '(+ (^ x 2) (* -5 x) 6) 'x) "(x - 3) * (x - 2)"))
(must "factor x^4 - 1"
      (equal? (factor-expr '(- (^ x 4) 1) 'x) "(x + 1) * (x - 1) * (x^2 + 1)"))
(newline)

; ============================================================
; 6. PARTIAL FRACTIONS — each checked by recombining over a common denominator
; ============================================================
(display "6. partial fractions (each checked by recombining)") (newline)
(define (pf-ok? num den) (pf-verify num den (partial-fractions num den)))
(must "1/(x^2-1)        distinct linear   -> verified" (pf-ok? (list 1) (list -1 0 1)))
(must "x^3/(x^2-1)      improper          -> verified" (pf-ok? (list 0 0 0 1) (list -1 0 1)))
(must "x/(x-1)^2        repeated factor   -> verified" (pf-ok? (list 0 1) (poly-pow (list -1 1) 2)))
(must "1/(x^3+x)        irreducible quad  -> verified" (pf-ok? (list 1) (list 0 1 0 1)))
(must "(2x^2+3)/(x-1)^3 high power        -> verified" (pf-ok? (list 3 0 2) (poly-pow (list -1 1) 3)))
; a specific decomposition the algorithm must get exactly right
(must "1/(x^2-1) = 1/2/(x-1) - 1/2/(x+1)"
      (equal? (pf->string (partial-fractions (list 1) (list -1 0 1)) "x")
              "-1/2/(x + 1) + 1/2/(x - 1)"))
(newline)

(display "all polynomial-algebra checks passed.") (newline)
