; 198-pratt-certificates.lisp -- constructive primality proofs (Pratt certificates).
;
; A Pratt certificate proves n is prime by exhibiting a witness a of multiplicative order
; exactly n-1: a^(n-1) = 1 (mod n) and a^((n-1)/q) /= 1 for each prime q | n-1, with a
; recursive certificate that every such q is itself prime, bottoming out at 2.  An
; independent verifier re-checks every node, so a forged certificate is rejected; and the
; result always agrees with the Miller-Rabin test.  `must` raises on failure.

(import "cas/pratt.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'pratt-check-failed)))
(define (all-agree lo hi) (cond ((> lo hi) #t) ((pratt-agrees? lo) (all-agree (+ lo 1) hi)) (else #f)))

(display "Pratt primality certificates") (newline) (newline)

(display "1. primes carry a verified certificate") (newline)
(display "    1009  -> ") (display (pratt->string 1009)) (display ", witness ") (display (witness-of 1009)) (newline)
(display "    104729 -> ") (display (pratt->string 104729)) (newline)
(must "7 is certified prime"      (pratt-prime? 7))
(must "1009 is certified prime"   (pratt-prime? 1009))
(must "9973 is certified prime"   (pratt-prime? 9973))
(must "104729 is certified prime" (pratt-prime? 104729))
(newline)

(display "2. composites have no certificate") (newline)
(must "9 is not prime"    (not (pratt-prime? 9)))
(must "1000 is not prime" (not (pratt-prime? 1000)))
(must "the Carmichael number 561 is not prime" (not (pratt-prime? 561)))
(must "121 = 11^2 is not prime" (not (pratt-prime? 121)))
(newline)

(display "3. cross-check: Pratt and Miller-Rabin agree everywhere on 2..200") (newline)
(must "the two primality methods agree on 2..200" (all-agree 2 200))
(newline)

(display "4. the verifier rejects a forged certificate") (newline)
(define good (pratt 1009))
(define bad (list 1009 3 (car (cdr (cdr good)))))   ; 3 is not a primitive root mod 1009
(must "the genuine certificate verifies"   (pratt-check good))
(must "a certificate with a false witness is rejected" (not (pratt-check bad)))
(newline)

(display "all Pratt certificate checks passed.") (newline)
