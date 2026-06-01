; 175-number-theory.lisp — exact integer number theory over the bignums.
;
; Extended Euclid, modular exponentiation/inverse, deterministic Miller-Rabin
; primality (witness set {2..37} is a proof below 3.3e24 and rejects Carmichael
; numbers), integer factorization by trial division (always exact and terminating),
; the Euler totient and divisor functions, the Chinese remainder construction, and
; multiplicative order.  Each fact is checked: factorizations multiply back to n with
; every part prime, inverses verify, CRT solutions satisfy each congruence, Euler's
; theorem holds.  `must` raises on failure.

(import "cas/numbertheory.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'nt-check-failed)))

(display "Exact integer number theory") (newline) (newline)

(display "1. factorization (reconstruction-certified)") (newline)
(display "    360      = ") (display (factor-int->string (factor-int 360))) (newline)
(display "    1234567  = ") (display (factor-int->string (factor-int 1234567))) (newline)
(display "    1000000  = ") (display (factor-int->string (factor-int 1000000))) (newline)
(must "360 factors verify"       (factor-int-ok? 360 (factor-int 360)))
(must "1234567 = 127 * 9721"     (factor-int-ok? 1234567 (factor-int 1234567)))
(must "1000000 = 2^6 * 5^6"      (factor-int-ok? 1000000 (factor-int 1000000)))
(newline)

(display "2. primality (Miller-Rabin, deterministic)") (newline)
(must "97 is prime"                       (prime? 97))
(must "1000003 is prime"                  (prime? 1000003))
(must "2^31 - 1 = 2147483647 is prime"    (prime? 2147483647))
(must "561 = 3*11*17 is composite (Carmichael)"  (not (prime? 561)))
(must "1729 Hardy-Ramanujan is composite (Carmichael)" (not (prime? 1729)))
(must "2^31 is composite"                 (not (prime? 2147483648)))
(newline)

(display "3. totient and Euler's theorem") (newline)
(must "phi(360) = 96"                 (= (totient 360) 96))
(must "phi(prime 1000003) = 1000002"  (= (totient 1000003) 1000002))
(must "phi(p*q) = (p-1)(q-1)"         (= (totient (* 127 9721)) (* 126 9720)))
(must "Euler: 2^phi(9) = 1 mod 9"     (= (mod-exp 2 (totient 9) 9) 1))
(must "Euler: 5^phi(21) = 1 mod 21"   (= (mod-exp 5 (totient 21) 21) 1))
(newline)

(display "4. divisor functions") (newline)
(must "tau(360) = 24"        (= (num-divisors 360) 24))
(must "sigma(360) = 1170"    (= (sigma1 360) 1170))
(must "sigma(6) = 12 (perfect)" (= (sigma1 6) 12))
(newline)

(display "5. modular inverse, CRT, order") (newline)
(must "3^-1 = 4 mod 11"             (= (mod-inverse 3 11) 4))
(must "inverse check 3*4 = 1 mod 11" (= (imod (* 3 (mod-inverse 3 11)) 11) 1))
(define x (crt2 2 3 3 5))
(must "CRT result = 8"              (= x 8))
(must "CRT satisfies x = 2 mod 3"   (= (imod x 3) 2))
(must "CRT satisfies x = 3 mod 5"   (= (imod x 5) 3))
(must "order of 2 mod 7 is 3"       (= (order-mod 2 7) 3))
(must "order divides phi: ord(3,7) | phi(7)" (= (imod (totient 7) (order-mod 3 7)) 0))
(newline)

(display "all number-theory checks passed.") (newline)
