; 184-two-squares.lisp -- writing integers as a sum of two squares (Fermat).
;
; A prime p is a sum of two squares iff p = 2 or p = 1 (mod 4); a positive integer is one
; iff every prime = 3 (mod 4) divides it to an even power.  Primes p = 1 (mod 4) are split
; by Cornacchia's method using a square root of -1 mod p, and a general n is assembled
; from its prime-power factors through the Brahmagupta-Fibonacci identity.  Every
; representation is gated by a^2 + b^2 = n.  `must` raises on failure.

(import "cas/twosquares.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'twosquares-check-failed)))
(define (sq a) (* a a))
(define (rep-sum n) (let ((r (two-squares n))) (+ (sq (car r)) (sq (cdr r)))))

(display "Sum of two squares (Fermat)") (newline) (newline)

(display "1. primes that are 1 mod 4 (Cornacchia)") (newline)
(display "    13  = ") (display (two-squares->string 13))  (newline)
(display "    97  = ") (display (two-squares->string 97))  (newline)
(display "    101 = ") (display (two-squares->string 101)) (newline)
(must "13 = a^2+b^2 with a^2+b^2 = 13"  (= (rep-sum 13) 13))
(must "97 representation certified"     (two-squares-ok? 97))
(must "101 representation certified"    (two-squares-ok? 101))
(newline)

(display "2. a large prime = 1 mod 4 (Cornacchia over Tonelli-Shanks)") (newline)
(display "    1000033 = ") (display (two-squares->string 1000033)) (newline)
(must "1000033 representation sums correctly" (= (rep-sum 1000033) 1000033))
(must "1000033 certified"                     (two-squares-ok? 1000033))
(newline)

(display "3. composites via the Brahmagupta-Fibonacci identity") (newline)
(display "    50  = ") (display (two-squares->string 50))  (newline)
(display "    325 = ") (display (two-squares->string 325)) (newline)
(display "    45  = ") (display (two-squares->string 45))  (newline)
(must "50 sums correctly"   (= (rep-sum 50) 50))
(must "325 sums correctly"  (= (rep-sum 325) 325))
(must "45 sums correctly"   (= (rep-sum 45) 45))
(must "9 = 3^2 + 0^2"       (= (rep-sum 9) 9))
(newline)

(display "4. existence test and honest refusals") (newline)
(must "3 has no representation"   (equal? (two-squares 3) 'none))
(must "21 has no representation"  (equal? (two-squares 21) 'none))
(must "existence flags correct"   (equal? (list (sum-of-two-squares? 3) (sum-of-two-squares? 21) (sum-of-two-squares? 45) (sum-of-two-squares? 50)) (list #f #f #t #t)))
(must "refusal on 7 is certified" (two-squares-ok? 7))
(newline)

(display "all two-squares checks passed.") (newline)
