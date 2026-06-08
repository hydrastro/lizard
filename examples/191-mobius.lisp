; 191-mobius.lisp -- the Mobius function and Dirichlet convolution.
;
; mu(n) is 1 at n=1, (-1)^k for a product of k distinct primes, and 0 when a square
; divides n.  Dirichlet convolution (f * g)(n) = sum_{d|n} f(d) g(n/d) makes the
; arithmetic functions a ring with identity epsilon(n) = [n=1], in which 1 and mu are
; inverses.  The structural identities are the certificates: mu * 1 = epsilon, phi * 1 = N,
; phi = id * mu, and a full Mobius-inversion round trip.  `must` raises on failure.

(import "cas/mobius.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'mobius-check-failed)))

(display "The Mobius function and Dirichlet convolution") (newline) (newline)

(display "1. values of mu") (newline)
(display "    mu(1..12) = ") (display (mobius-list 13)) (newline)
(must "mu(1) = 1"        (= (mobius 1) 1))
(must "mu(6) = 1 (two primes)"   (= (mobius 6) 1))
(must "mu(30) = -1 (three primes)" (= (mobius 30) -1))
(must "mu(4) = 0 (square factor)"  (= (mobius 4) 0))
(must "mu(105) = -1"     (= (mobius 105) -1))
(newline)

(display "2. the Mertens summatory function") (newline)
(display "    M(1..12) = ") (display (mertens-list 13)) (newline)
(must "M(12) = -2"       (= (mertens 12) -2))
(newline)

(display "3. convolution identities (the certificates)") (newline)
(must "mu * 1 = epsilon on 1..40"  (mu-1-is-epsilon? 40))
(must "phi * 1 = N on 1..40"       (phi-1-is-N? 40))
(must "divisor sum of phi at 12 is 12" (= (dirichlet totient one 12) 12))
(must "phi = id * mu at 36"        (= (dirichlet id mobius 36) (totient 36)))
(must "phi = id * mu at 100"       (= (dirichlet id mobius 100) (totient 100)))
(newline)

(display "4. Mobius inversion round-trip") (newline)
(display "    recover f from its divisor-sum (f * 1) via (g * mu)") (newline)
(must "inversion recovers f(n) = n^2 on 1..25"  (inversion-ok? (lambda (n) (* n n)) 25))
(must "inversion recovers f(n) = 1 on 1..25"    (inversion-ok? one 25))
(must "inversion recovers the totient on 1..25" (inversion-ok? totient 25))
(newline)

(display "all Mobius checks passed.") (newline)
